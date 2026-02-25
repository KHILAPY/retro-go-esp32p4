/*
 * JIT Test Suite for GBA Dynamic Recompiler
 * 
 * Standalone test for verifying JIT functionality.
 */

#include "jit_core.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "JIT_TEST";

/* Mock memory for testing */
static uint8_t s_test_memory[0x100000];

/* Mock GBA memory access functions */
uint32_t gba_mem_read32(uint32_t addr)
{
    addr &= 0xFFFFF;
    return *(uint32_t *)(s_test_memory + addr);
}

void gba_mem_write32(uint32_t addr, uint32_t value)
{
    addr &= 0xFFFFF;
    *(uint32_t *)(s_test_memory + addr) = value;
}

uint16_t gba_mem_read16(uint32_t addr)
{
    addr &= 0xFFFFF;
    return *(uint16_t *)(s_test_memory + addr);
}

void gba_mem_write16(uint32_t addr, uint16_t value)
{
    addr &= 0xFFFFF;
    *(uint16_t *)(s_test_memory + addr) = value;
}

uint8_t gba_mem_read8(uint32_t addr)
{
    addr &= 0xFFFFF;
    return s_test_memory[addr];
}

void gba_mem_write8(uint32_t addr, uint8_t value)
{
    addr &= 0xFFFFF;
    s_test_memory[addr] = value;
}

/* --------------------------------------------------------------------- */
/*  Test Cases                                                           */
/* --------------------------------------------------------------------- */

static int test_jit_init(void)
{
    ESP_LOGI(TAG, "Test: JIT Initialization");
    
    int ret = jit_init(NULL);
    if (ret != 0) {
        ESP_LOGE(TAG, "  FAILED: jit_init returned %d", ret);
        return -1;
    }
    
    jit_print_stats();
    jit_deinit();
    
    ESP_LOGI(TAG, "  PASSED");
    return 0;
}

static int test_add_instruction(void)
{
    ESP_LOGI(TAG, "Test: ADD Instruction Translation");
    
    jit_init(NULL);
    
    memset(s_test_memory, 0, sizeof(s_test_memory));
    
    uint32_t *code = (uint32_t *)s_test_memory;
    
    code[0] = 0xE2810002;
    code[1] = 0xEA000000;
    
    gba_cpu_state_t cpu;
    memset(&cpu, 0, sizeof(cpu));
    cpu.reg[1] = 10;
    cpu.reg[15] = 0;
    
    block_entry_t *block = jit_block_compile(0, false);
    if (block == NULL) {
        ESP_LOGE(TAG, "  FAILED: Block compilation failed");
        jit_deinit();
        return -1;
    }
    
    uint32_t new_pc = jit_execute_block(block, &cpu);
    
    if (cpu.reg[0] != 12) {
        ESP_LOGE(TAG, "  FAILED: Expected R0=12, got R0=%u", cpu.reg[0]);
        jit_deinit();
        return -1;
    }
    
    ESP_LOGI(TAG, "  R0 = %u (expected 12)", cpu.reg[0]);
    ESP_LOGI(TAG, "  PASSED");
    
    jit_deinit();
    return 0;
}

static int test_sub_instruction(void)
{
    ESP_LOGI(TAG, "Test: SUB Instruction Translation");
    
    jit_init(NULL);
    
    memset(s_test_memory, 0, sizeof(s_test_memory));
    
    uint32_t *code = (uint32_t *)s_test_memory;
    
    code[0] = 0xE0410002;
    code[1] = 0xEA000000;
    
    gba_cpu_state_t cpu;
    memset(&cpu, 0, sizeof(cpu));
    cpu.reg[1] = 20;
    cpu.reg[2] = 8;
    cpu.reg[15] = 0;
    
    block_entry_t *block = jit_block_compile(0, false);
    if (block == NULL) {
        ESP_LOGE(TAG, "  FAILED: Block compilation failed");
        jit_deinit();
        return -1;
    }
    
    jit_execute_block(block, &cpu);
    
    if (cpu.reg[0] != 12) {
        ESP_LOGE(TAG, "  FAILED: Expected R0=12, got R0=%u", cpu.reg[0]);
        jit_deinit();
        return -1;
    }
    
    ESP_LOGI(TAG, "  R0 = %u (expected 12)", cpu.reg[0]);
    ESP_LOGI(TAG, "  PASSED");
    
    jit_deinit();
    return 0;
}

static int test_mov_instruction(void)
{
    ESP_LOGI(TAG, "Test: MOV Instruction Translation");
    
    jit_init(NULL);
    
    memset(s_test_memory, 0, sizeof(s_test_memory));
    
    uint32_t *code = (uint32_t *)s_test_memory;
    
    code[0] = 0xE3A00042;
    code[1] = 0xEA000000;
    
    gba_cpu_state_t cpu;
    memset(&cpu, 0, sizeof(cpu));
    cpu.reg[15] = 0;
    
    block_entry_t *block = jit_block_compile(0, false);
    if (block == NULL) {
        ESP_LOGE(TAG, "  FAILED: Block compilation failed");
        jit_deinit();
        return -1;
    }
    
    jit_execute_block(block, &cpu);
    
    if (cpu.reg[0] != 0x42) {
        ESP_LOGE(TAG, "  FAILED: Expected R0=0x42, got R0=0x%08X", cpu.reg[0]);
        jit_deinit();
        return -1;
    }
    
    ESP_LOGI(TAG, "  R0 = 0x%08X (expected 0x42)", cpu.reg[0]);
    ESP_LOGI(TAG, "  PASSED");
    
    jit_deinit();
    return 0;
}

static int test_and_instruction(void)
{
    ESP_LOGI(TAG, "Test: AND Instruction Translation");
    
    jit_init(NULL);
    
    memset(s_test_memory, 0, sizeof(s_test_memory));
    
    uint32_t *code = (uint32_t *)s_test_memory;
    
    code[0] = 0xE0010002;
    code[1] = 0xEA000000;
    
    gba_cpu_state_t cpu;
    memset(&cpu, 0, sizeof(cpu));
    cpu.reg[1] = 0xFF;
    cpu.reg[2] = 0x0F;
    cpu.reg[15] = 0;
    
    block_entry_t *block = jit_block_compile(0, false);
    if (block == NULL) {
        ESP_LOGE(TAG, "  FAILED: Block compilation failed");
        jit_deinit();
        return -1;
    }
    
    jit_execute_block(block, &cpu);
    
    if (cpu.reg[0] != 0x0F) {
        ESP_LOGE(TAG, "  FAILED: Expected R0=0x0F, got R0=0x%08X", cpu.reg[0]);
        jit_deinit();
        return -1;
    }
    
    ESP_LOGI(TAG, "  R0 = 0x%08X (expected 0x0F)", cpu.reg[0]);
    ESP_LOGI(TAG, "  PASSED");
    
    jit_deinit();
    return 0;
}

static int test_orr_instruction(void)
{
    ESP_LOGI(TAG, "Test: ORR Instruction Translation");
    
    jit_init(NULL);
    
    memset(s_test_memory, 0, sizeof(s_test_memory));
    
    uint32_t *code = (uint32_t *)s_test_memory;
    
    code[0] = 0xE1810002;
    code[1] = 0xEA000000;
    
    gba_cpu_state_t cpu;
    memset(&cpu, 0, sizeof(cpu));
    cpu.reg[1] = 0xF0;
    cpu.reg[2] = 0x0F;
    cpu.reg[15] = 0;
    
    block_entry_t *block = jit_block_compile(0, false);
    if (block == NULL) {
        ESP_LOGE(TAG, "  FAILED: Block compilation failed");
        jit_deinit();
        return -1;
    }
    
    jit_execute_block(block, &cpu);
    
    if (cpu.reg[0] != 0xFF) {
        ESP_LOGE(TAG, "  FAILED: Expected R0=0xFF, got R0=0x%08X", cpu.reg[0]);
        jit_deinit();
        return -1;
    }
    
    ESP_LOGI(TAG, "  R0 = 0x%08X (expected 0xFF)", cpu.reg[0]);
    ESP_LOGI(TAG, "  PASSED");
    
    jit_deinit();
    return 0;
}

static int test_eor_instruction(void)
{
    ESP_LOGI(TAG, "Test: EOR Instruction Translation");
    
    jit_init(NULL);
    
    memset(s_test_memory, 0, sizeof(s_test_memory));
    
    uint32_t *code = (uint32_t *)s_test_memory;
    
    code[0] = 0xE0210002;
    code[1] = 0xEA000000;
    
    gba_cpu_state_t cpu;
    memset(&cpu, 0, sizeof(cpu));
    cpu.reg[1] = 0xFF;
    cpu.reg[2] = 0x0F;
    cpu.reg[15] = 0;
    
    block_entry_t *block = jit_block_compile(0, false);
    if (block == NULL) {
        ESP_LOGE(TAG, "  FAILED: Block compilation failed");
        jit_deinit();
        return -1;
    }
    
    jit_execute_block(block, &cpu);
    
    if (cpu.reg[0] != 0xF0) {
        ESP_LOGE(TAG, "  FAILED: Expected R0=0xF0, got R0=0x%08X", cpu.reg[0]);
        jit_deinit();
        return -1;
    }
    
    ESP_LOGI(TAG, "  R0 = 0x%08X (expected 0xF0)", cpu.reg[0]);
    ESP_LOGI(TAG, "  PASSED");
    
    jit_deinit();
    return 0;
}

static int test_block_lookup(void)
{
    ESP_LOGI(TAG, "Test: Block Lookup");
    
    jit_init(NULL);
    
    memset(s_test_memory, 0, sizeof(s_test_memory));
    uint32_t *code = (uint32_t *)s_test_memory;
    code[0] = 0xE3A00042;
    code[1] = 0xEA000000;
    
    block_entry_t *block1 = jit_block_compile(0, false);
    if (block1 == NULL) {
        ESP_LOGE(TAG, "  FAILED: First compilation failed");
        jit_deinit();
        return -1;
    }
    
    block_entry_t *block2 = jit_block_lookup(0, false);
    if (block2 == NULL) {
        ESP_LOGE(TAG, "  FAILED: Block lookup failed");
        jit_deinit();
        return -1;
    }
    
    if (block1 != block2) {
        ESP_LOGE(TAG, "  FAILED: Block pointers don't match");
        jit_deinit();
        return -1;
    }
    
    ESP_LOGI(TAG, "  PASSED");
    
    jit_deinit();
    return 0;
}

/* --------------------------------------------------------------------- */
/*  Test Runner                                                          */
/* --------------------------------------------------------------------- */

void jit_run_tests(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "GBA JIT Test Suite");
    ESP_LOGI(TAG, "========================================");
    
    int passed = 0;
    int failed = 0;
    
    if (test_jit_init() == 0) passed++; else failed++;
    if (test_add_instruction() == 0) passed++; else failed++;
    if (test_sub_instruction() == 0) passed++; else failed++;
    if (test_mov_instruction() == 0) passed++; else failed++;
    if (test_and_instruction() == 0) passed++; else failed++;
    if (test_orr_instruction() == 0) passed++; else failed++;
    if (test_eor_instruction() == 0) passed++; else failed++;
    if (test_block_lookup() == 0) passed++; else failed++;
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Results: %d passed, %d failed", passed, failed);
    ESP_LOGI(TAG, "========================================");
}
