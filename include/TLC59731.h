/*
 * TLC59731.h
 *
 *  Created on: 14. sij 2019.
 *      Author: Admin
 */

#ifndef COMPONENTS_LEDDRIVER_INCLUDE_TLC59731_H_
#define COMPONENTS_LEDDRIVER_INCLUDE_TLC59731_H_

#include <stdio.h>
#include "stdbool.h"
#include "string.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "driver/rmt.h"

#define RMT_CHANNEL0_TICK_US 2

#define INTERVAL_US(us) ((us) / RMT_CHANNEL0_TICK_US)

#define TLC59731_PIN 27
#define TLC59731_T_CYCLE 50 //50us

typedef struct{
	uint8_t r;
	uint8_t g;
	uint8_t b;
}ledRGB;

void tlc59731_init();
void tlc59731_setGrayscale(ledRGB* gs, uint8_t countLEDs);
void tlc59731_release();

#endif /* COMPONENTS_LEDDRIVER_INCLUDE_TLC59731_H_ */
