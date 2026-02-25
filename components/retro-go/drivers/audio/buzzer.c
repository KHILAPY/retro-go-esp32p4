/*

Audio driver for piezoelectric buzzer
=====================================
Getting a piezoelectric buzzer to produce Pulse-Code Modulation (PCM) audio without a Digital-to-Analog-Converter (DAC) isn't trivial.
It also doesn't sound as great or loud as a real speaker. But it sounds okay when done properly, and it's better than nothing.

Typically, a buzzer is driven "high" and "low" at a certain frequency to produce a square wave of an audio signal.
For example: if the buzzer is turned on and off at 440Hz, an "A" note (with a sharp timbre, due to the square wave) is heard.

The position of the piezoelectric buzzer is actually proportional to the output voltage on RG_AUDIO_USE_BUZZER_PIN.
So a voltage of 1.65V of 3.3V total will cause the position of the buzzer's ceramic disk to be halfway.

This can be used to output audio, but without a DAC, the only way to approximate an analog output voltage in the range of 0 and 3.3V
on the pin is by quickly turning that pin on and off, using the Pulse-Width-Modulation (PWM) facilities of the ESP32 microcontroller.

For example: to get an average voltage of 1.1V, the PWM can be configured for a high frequency (say 32kHz) with a duty cycle of 33.33%.
Then the pin will be high 1/3 of the time and low 2/3 of of the time, resulting in an average output voltage of 1.1V.

To produce acceptable sound, the duty cycle of the PWM output should be adjusted at least as often as the samplerate of
the PCM input audio, typically 22050-48000 Hz. It should also be adjusted at precisely the right moment.

To achieve this timing precision, the General Purpose Timer provisions of the ESP32 microcontroller are used.
It's configured to trigger an interrupt handler at the right moment, which then sets the PWM duty cycle.

One challenge is that the audio should always keep playing, even when the chip is very busy rendering a frame or parsing
user input. To achieve this, audio samples are buffered in a queue, and the timing of each frame is dynamically adjusted
to ensure the queue never underflows, nor overflows.

Note that there are some restrictions on how high the PWM frequency can be, and a tradeoff with the precision of the duty cycle.

*/

#include "rg_system.h"

#if RG_AUDIO_USE_BUZZER_PIN
#include "rg_audio.h"

#ifndef ESP_PLATFORM
#error "Audio buzzer support can only be built inside esp-idf because it uses its LEDC, TIMER and interrupt handlers!"
#endif

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <driver/gptimer.h>
#include <driver/ledc.h>
#include <inttypes.h>

#define BOOSTVOLUME                   3
#define AVERAGE_NR_OF_SAMPLES_NEEDED 25
#define MS_OF_CACHED_SAMPLES         50

#define SOURCE_CLOCK_MIN_DIVIDER	  2
#define SOURCE_CLOCK_FREQUENCY        80000000
#define SOURCE_CLOCK_MAX_FREQUENCY    SOURCE_CLOCK_FREQUENCY/SOURCE_CLOCK_MIN_DIVIDER
#define LEDC_PWM_SPEED_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_PWM_CHANNEL              LEDC_CHANNEL_0
#define LEDC_PWM_TIMER                LEDC_TIMER_0

#define ESP_MAX_PWM_FREQ_1_BIT        40000000
#define ESP_MAX_PWM_FREQ_2_BIT        20000000
#define ESP_MAX_PWM_FREQ_3_BIT        10000000
#define ESP_MAX_PWM_FREQ_4_BIT        5000000
#define ESP_MAX_PWM_FREQ_5_BIT        2500000
#define ESP_MAX_PWM_FREQ_6_BIT        1250000
#define ESP_MAX_PWM_FREQ_7_BIT        625000
#define ESP_MAX_PWM_FREQ_8_BIT        312500
#define ESP_MAX_PWM_FREQ_9_BIT        156250
#define ESP_MAX_PWM_FREQ_10_BIT       78125
#define ESP_MAX_PWM_FREQ_11_BIT       39062
#define ESP_MAX_PWM_FREQ_12_BIT       19531
#define ESP_MAX_PWM_FREQ_13_BIT       9765
#define ESP_MAX_PWM_FREQ_14_BIT       4882

#define NOTE_A 440
#define NOTE_C 523
#define NOTE_HIGH_C 1046
#define MAX_AUDIBLE_NOTE 16000

int underflows = 0;
int overflows = 0;
QueueHandle_t sampleQueue;

float averageSamplesNeeded = 0;
int averageSamplesCount = 1;
int approxRollingAverage(float avg, int input) {
  averageSamplesCount = RG_MIN(averageSamplesCount++, AVERAGE_NR_OF_SAMPLES_NEEDED);
  avg -= avg/averageSamplesCount;
  avg += input/averageSamplesCount;
  return avg;
}


#ifdef PLAY_SINE_AS_TEST

#include <math.h>

#define SINE_AMPLITUDE 32767

int sinePeriod;
int sinePosition;
int16_t *sineBuffer;

void precompute_sine_wave(int sampleRate)
{
    sinePosition = 0;
    sinePeriod = 11 * sampleRate / NOTE_A;
    sineBuffer = malloc(sinePeriod * 2 * sizeof(int16_t));
    if (!sineBuffer) {
        RG_LOGE("could not allocate memory for sineBuffer");
        return;
    }

    int32_t phase = 0;
    int bufferPos = 0;
    for (int i = 0; i < sinePeriod; i++) {
        float sample = sinf(phase * M_PI * 2 / sampleRate) * SINE_AMPLITUDE;
        sineBuffer[bufferPos] = sample;
        sineBuffer[bufferPos+1] = sample;
        phase += NOTE_A;
        bufferPos += 2;
    }
}

#endif

static gptimer_handle_t gptimer = NULL;
static int g_clipbits = 0;

static bool IRAM_ATTR buzzer_interrupt_handler(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    int clipbits = g_clipbits;

    BaseType_t xTaskWokenByReceive = pdFALSE;
    int16_t sample_int16;
    if (xQueueReceiveFromISR(sampleQueue, (void *) &sample_int16, &xTaskWokenByReceive) == pdPASS) {
        int32_t sample = sample_int16;
        sample = sample + 32768;
        sample = sample >> clipbits;
        ledc_set_duty(LEDC_PWM_SPEED_MODE, LEDC_PWM_CHANNEL, sample);
        ledc_update_duty(LEDC_PWM_SPEED_MODE, LEDC_PWM_CHANNEL);
    } else {
        underflows++;
    }

    return (xTaskWokenByReceive == pdTRUE);
}

void rg_buzzer_statistics(void *args)
{
	int cacheSamples = (int) args;
    while (sampleQueue)
    {
        RG_LOGD("underflows %d, overflows %d, averageSamplesNeeded: %.6f, sampleQueue: [0,%d,%d]", underflows, overflows, averageSamplesNeeded, uxQueueMessagesWaiting(sampleQueue), cacheSamples*2);
        rg_task_delay(500);
    }
}

static bool buzzer_init(int device, int sampleRate)
{
    RG_LOGI("Configuring buzzer for %d Hz samplerate", sampleRate);

    underflows = 0;
    overflows = 0;
    int cacheSamples = (sampleRate/1000)*MS_OF_CACHED_SAMPLES;

#ifdef PLAY_SINE_AS_TEST
    precompute_sine_wave(sampleRate);
#endif

    sampleQueue = xQueueCreate(cacheSamples*2, sizeof(int16_t*));
    if (!sampleQueue) {
        RG_LOGE("could not create sampleQueue");
        return false;
    }

    rg_task_create("rg_buzzer_statistics", &rg_buzzer_statistics, (void*) cacheSamples, 3 * 1024, RG_TASK_PRIORITY_5, -1);

    int freq = sampleRate;
    while (freq < MAX_AUDIBLE_NOTE) {
        freq *= 2;
        RG_LOGI("sampleRate < %d kHz would cause audible sound, adjusting PWM frequency to %d", MAX_AUDIBLE_NOTE, freq);
    }

    int pwm_resolution = 8;
    if (freq < ESP_MAX_PWM_FREQ_13_BIT)
        pwm_resolution = 13;
    else if (freq < ESP_MAX_PWM_FREQ_12_BIT)
        pwm_resolution = 12;
    else if (freq < ESP_MAX_PWM_FREQ_11_BIT)
        pwm_resolution = 11;
    else if (freq < ESP_MAX_PWM_FREQ_10_BIT)
        pwm_resolution = 10;
    else if (freq < ESP_MAX_PWM_FREQ_9_BIT)
        pwm_resolution = 9;

    int clipbits = 16 - pwm_resolution;
    g_clipbits = clipbits;

    RG_LOGI("Minimal PWM duty resolution for buzzer is %d so %d bits remain for duty cycle depth", pwm_resolution, clipbits);

    ledc_timer_config_t ledc_timer = {
        .duty_resolution = pwm_resolution,
        .freq_hz = NOTE_A,
        .speed_mode = LEDC_PWM_SPEED_MODE,
        .timer_num = LEDC_PWM_TIMER,
        .clk_cfg = LEDC_USE_APB_CLK,
    };
    esp_err_t result = ledc_timer_config(&ledc_timer);
    RG_LOGI("ledc_timer_config returns: %d", result);

    ledc_channel_config_t ledc_channel = {
        .channel    = LEDC_PWM_CHANNEL,
        .duty       = 1 << (pwm_resolution-1),
        .gpio_num   = RG_AUDIO_USE_BUZZER_PIN,
        .speed_mode = LEDC_PWM_SPEED_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_PWM_TIMER
    };
    result = ledc_channel_config(&ledc_channel);
    RG_LOGI("ledc_channel_config returns: %d", result);

    ledc_set_duty(LEDC_PWM_SPEED_MODE, LEDC_PWM_CHANNEL, 0);
    ledc_update_duty(LEDC_PWM_SPEED_MODE, LEDC_PWM_CHANNEL);

    ledc_timer.freq_hz = freq;
    ledc_timer_config(&ledc_timer);

    RG_LOGD("creating timer with interrupt handler for buzzer");

    int foundDivider;
    for (foundDivider=SOURCE_CLOCK_MAX_FREQUENCY/sampleRate; foundDivider>1; foundDivider--) {
        if ((SOURCE_CLOCK_MAX_FREQUENCY % foundDivider == 0) && (SOURCE_CLOCK_MAX_FREQUENCY / foundDivider % sampleRate) == 0) {
	        RG_LOGD("Found sample playing timer divider that results in an integer alarmTrigger: %d", foundDivider);
	        break;
	    }
    }
    if (foundDivider < SOURCE_CLOCK_MIN_DIVIDER) {
        RG_LOGI("Could not find sample playing divider that results in an integer alarmTrigger, defaulting to slightly off alarmTrigger, which also sounds fine...");
        foundDivider = SOURCE_CLOCK_MIN_DIVIDER;
    }

    int alarmTrigger = (SOURCE_CLOCK_FREQUENCY / foundDivider) / sampleRate;

    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = SOURCE_CLOCK_FREQUENCY / foundDivider,
    };
    result = gptimer_new_timer(&timer_config, &gptimer);
    RG_LOGD("gptimer_new_timer result: %d", result);

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = alarmTrigger,
        .flags.auto_reload_on_alarm = true,
    };
    result = gptimer_set_alarm_action(gptimer, &alarm_config);
    RG_LOGD("gptimer_set_alarm_action result: %d", result);

    gptimer_event_callbacks_t cbs = {
        .on_alarm = buzzer_interrupt_handler,
    };
    result = gptimer_register_event_callbacks(gptimer, &cbs, NULL);
    RG_LOGD("gptimer_register_event_callbacks result: %d", result);

    result = gptimer_enable(gptimer);
    RG_LOGD("gptimer_enable result: %d", result);

    result = gptimer_start(gptimer);
    RG_LOGD("gptimer_start result: %d", result);

    return true;
}

static bool buzzer_deinit(void)
{
    RG_LOGI("stopping buzzer");
    
    if (gptimer) {
        gptimer_stop(gptimer);
        gptimer_disable(gptimer);
        gptimer_del_timer(gptimer);
        gptimer = NULL;
    }

    vQueueDelete(sampleQueue);
    sampleQueue = NULL;

#ifdef PLAY_SINE_AS_TEST
    if (sineBuffer) free(sineBuffer);
#endif

    return true;
}


static bool buzzer_submit(const rg_audio_frame_t *frames, size_t count)
{
    float volumeFactor = rg_audio_get_mute() ? 0.f : (rg_audio_get_volume() * 0.01f) * BOOSTVOLUME;
    int sampleRate = rg_audio_get_sample_rate();

    for (size_t i = 0; i < count; ++i)
    {
        int16_t left = frames[i].left * volumeFactor;

#ifdef PLAY_SINE_AS_TEST
        left = sineBuffer[sinePosition];
        sinePosition+=2;
        if (sinePosition >= sinePeriod*2)
            sinePosition = 0;
#endif

        if (xQueueSend(sampleQueue, (void*)&left, 0) != pdPASS)
            overflows++;
    }

    int uSleepTime = count * (1000000.f / sampleRate);
    int usDeltaToCorrectSamplesInBuffer = (averageSamplesNeeded * 1000000.f ) / sampleRate;
    rg_usleep((uint32_t)(RG_MAX(0,uSleepTime - usDeltaToCorrectSamplesInBuffer)));

    int cacheSamples = (sampleRate/1000)*MS_OF_CACHED_SAMPLES;
    int samplesNeeded = cacheSamples - uxQueueMessagesWaiting(sampleQueue);
    averageSamplesNeeded = approxRollingAverage(averageSamplesNeeded, samplesNeeded);

    return true;
}

static bool buzzer_set_mute(bool mute)
{
    RG_LOGI("setting mute to %d", mute);
    if (mute)
    {
        if (gptimer) {
            gptimer_stop(gptimer);
            gptimer_disable(gptimer);
        }
        ledc_set_duty(LEDC_PWM_SPEED_MODE, LEDC_PWM_CHANNEL, 0);
        ledc_update_duty(LEDC_PWM_SPEED_MODE, LEDC_PWM_CHANNEL);
    }
    else
    {
        ledc_set_duty(LEDC_PWM_SPEED_MODE, LEDC_PWM_CHANNEL, 0);
        ledc_update_duty(LEDC_PWM_SPEED_MODE, LEDC_PWM_CHANNEL);
        if (gptimer) {
            gptimer_enable(gptimer);
            gptimer_start(gptimer);
        }
    }

    return true;
}

const rg_audio_driver_t rg_audio_driver_buzzer = {
    .name = "buzzer",
    .init = buzzer_init,
    .deinit = buzzer_deinit,
    .submit = buzzer_submit,
    .set_mute = buzzer_set_mute,
    .set_volume = NULL,
    .set_sample_rate = NULL,
    .get_error = NULL,
};

#endif
