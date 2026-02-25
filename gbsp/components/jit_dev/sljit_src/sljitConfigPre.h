/*
 * SLJIT Pre-Configuration for ESP32-P4
 * 
 * This file is included before the main SLJIT configuration
 * to set up ESP32-P4 specific settings.
 */

#ifndef SLJIT_CONFIG_PRE_H_
#define SLJIT_CONFIG_PRE_H_

/* Include ESP32-P4 specific configuration */
#include "sljitConfigEsp32P4.h"

/* Indicate that we have a pre-config header */
#define SLJIT_HAVE_CONFIG_PRE 1

#endif /* SLJIT_CONFIG_PRE_H_ */
