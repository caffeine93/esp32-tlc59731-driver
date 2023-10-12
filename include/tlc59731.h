#ifndef TLC59731_H_
#define TLC59731_H_

#include <stdio.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "sdkconfig.h"

#include "driver/rmt.h"

typedef struct {
        uint8_t pin;
        uint8_t rmt_ch;
} tlc59731_handle_t;

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} led_rgb_t;

esp_err_t tlc59731_init(tlc59731_handle_t **h, uint8_t pin, uint8_t rmt_ch);
esp_err_t tlc59731_set_grayscale(tlc59731_handle_t *h, led_rgb_t *gs, uint8_t count_leds);
esp_err_t tlc59731_release(tlc59731_handle_t **h);

#endif /* TLC59731_H_ */
