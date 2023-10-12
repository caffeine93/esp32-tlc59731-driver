/***************************************************************
 * Name:      tlc59731
 * Purpose:   Code for TLC59731 driver implementation on ESP-IDF
 * Author:    Luka Culic Viskota (luka.culic.viskota@gmail.com)
 * Created:   Monday 2019-01-14-09.45.08
 * Copyright: Luka Culic Viskota (https://microcafe.co)
 * License:   GNU General Public License v2.0
 **************************************************************/

#include <string.h>
#include "driver/gpio.h"
#include "driver/rmt.h"

#include "tlc59731.h"

#define RMT_CHANNEL_TICK_US 2

#define INTERVAL_US(us) ((us) / RMT_CHANNEL_TICK_US)

#define TLC59731_T_CYCLE 50 /* 50us */

#define TLC59731_CMD_WRITE 0x3A


static inline rmt_config_t tlc59731_get_rmt_config(uint8_t pin, uint8_t rmt_ch)
{
	rmt_config_t rmt_cfg;
	rmt_cfg.rmt_mode = RMT_MODE_TX;
	rmt_cfg.channel = rmt_ch;
	rmt_cfg.gpio_num = pin;
	rmt_cfg.mem_block_num = 1;
	rmt_cfg.tx_config.loop_en = 0;
	rmt_cfg.tx_config.carrier_en = 0;
	rmt_cfg.tx_config.idle_output_en = 1;
	rmt_cfg.tx_config.idle_level = 0;
	/*
	 * The RMT source clock is typically APB CLK of 80MHz, thus
	 * 80MHz / 160 = 500KHz, and thus 1 tick equals
	 * 1 / 500.000Hz = 2us
	 */
	rmt_cfg.clk_div = 160;

	return rmt_cfg;
}

static inline rmt_item32_t tlc59731_get_EOS_in_rmt_item(void)
{
	rmt_item32_t rmt_EOS = {
			{{ INTERVAL_US(TLC59731_T_CYCLE*2), 0,
				INTERVAL_US(TLC59731_T_CYCLE*2), 0 }}
	};

	return rmt_EOS;
}

static inline rmt_item32_t tlc59731_get_GSLAT_in_rmt_item(void)
{
	rmt_item32_t rmt_GSLAT = {
			{{ INTERVAL_US(TLC59731_T_CYCLE*4), 0,
				INTERVAL_US(TLC59731_T_CYCLE*4), 0 }}
	};

	return rmt_GSLAT;
}

static rmt_item32_t *tlc59731_get_bit_in_rmt_item(uint8_t bit)
{
	rmt_item32_t *rmt_bit = malloc(sizeof(*rmt_bit) * 2);
	if (!rmt_bit)
		return NULL;

	rmt_bit[0].level0 = 1;
	rmt_bit[0].duration0 = INTERVAL_US(TLC59731_T_CYCLE/5);
	rmt_bit[0].level1 = 0;
	rmt_bit[0].duration1 = INTERVAL_US(TLC59731_T_CYCLE/5);

	if (bit & 0x80)
		rmt_bit[1].level0 = 1;
	else
		rmt_bit[1].level0 = 0;

	rmt_bit[1].duration0 = INTERVAL_US(TLC59731_T_CYCLE/5);
	rmt_bit[1].level1 = 0;
	rmt_bit[1].duration1 = INTERVAL_US((TLC59731_T_CYCLE/5)*3);

	return rmt_bit;
}

static rmt_item32_t *tlc59731_get_byte_in_rmt_item(uint8_t data)
{
	rmt_item32_t *rmt_write_cmd = NULL;
	rmt_item32_t *rmt_bit = NULL;
	uint8_t tmp_bit;

	rmt_write_cmd = malloc(sizeof(*rmt_write_cmd) * 2 * 8);
	if (!rmt_write_cmd)
		return NULL;

	for (uint8_t i = 0; i < 8; i++) {
		tmp_bit = data << i;
		rmt_bit = tlc59731_get_bit_in_rmt_item(tmp_bit & 0x80);
		memcpy(rmt_write_cmd + (i * 2), rmt_bit, sizeof(*rmt_write_cmd) * 2);

		free(rmt_bit);
	}

	return rmt_write_cmd;
}

static inline rmt_item32_t *tlc59731_get_write_cmd_in_rmt_item(void)
{
	return tlc59731_get_byte_in_rmt_item(TLC59731_CMD_WRITE);
}

static rmt_item32_t *tlc59731_get_grayscale_in_rmt_item(uint8_t gs[3])
{
	rmt_item32_t *rmt_grayscale = NULL;
	rmt_item32_t *tmp_byte = NULL;

	rmt_grayscale = malloc(sizeof(*rmt_grayscale) * 3 * 8 * 2);
	if (!rmt_grayscale)
		return NULL;

	for (uint8_t i = 0; i < 3; i++) {
		tmp_byte = tlc59731_get_byte_in_rmt_item(gs[i]);
		memcpy(rmt_grayscale + (i * 2) * 8, tmp_byte,
				sizeof(*rmt_grayscale) * 2 * 8);
		free(tmp_byte);
	}

	return rmt_grayscale;
}

esp_err_t tlc59731_init(tlc59731_handle_t **h, uint8_t pin, uint8_t rmt_ch)
{
        rmt_config_t rmt_cfg = tlc59731_get_rmt_config(pin, rmt_ch);
	esp_err_t ret;

	if (!h)
		return ESP_ERR_INVALID_ARG;

	if (*h)
		return ESP_ERR_INVALID_STATE;

	ret = gpio_set_direction(pin, GPIO_MODE_OUTPUT);
	if (ret != ESP_OK)
		return ret;

	ret = gpio_set_level(pin, 0);
	if (ret != ESP_OK)
		return ret;

	ret = rmt_config(&rmt_cfg);
	if (ret != ESP_OK)
		return ret;

	ret = rmt_driver_install(rmt_cfg.channel, 0, 0);
	if (ret != ESP_OK)
		return ret;

	*h = malloc(sizeof(**h));
	if (!(*h)) {
		rmt_driver_uninstall(rmt_cfg.channel);
		return ESP_ERR_NO_MEM;
	}

	(*h)->pin = pin;
	(*h)->rmt_ch = rmt_ch;

	return ESP_OK;
}

esp_err_t tlc59731_set_grayscale(tlc59731_handle_t *h, led_rgb_t *gs, uint8_t count_leds)
{
	rmt_item32_t packet[(64 + 1) * count_leds];
	rmt_item32_t *rmt_grayscale = NULL;
	rmt_item32_t *rmt_write_cmd = NULL;
	rmt_item32_t rmt_EOS = tlc59731_get_EOS_in_rmt_item();
	rmt_item32_t rmt_GSLAT = tlc59731_get_GSLAT_in_rmt_item();

	uint8_t tmp_gs[3] = {0};

	if (!h || !gs || !count_leds)
		return ESP_ERR_INVALID_ARG;

	rmt_write_cmd = tlc59731_get_write_cmd_in_rmt_item();
	if (!rmt_write_cmd)
		return ESP_FAIL;

	for (uint8_t i = 0; i < count_leds; i++) {
		memcpy(packet + (i * (64 + 1)), rmt_write_cmd,
				sizeof(*packet) * 8 * 2);

		tmp_gs[0] = gs[i].r;
		tmp_gs[1] = gs[i].g;
		tmp_gs[2] = gs[i].b;

		rmt_grayscale = tlc59731_get_grayscale_in_rmt_item(tmp_gs);
		if (!rmt_grayscale) {
			free(rmt_write_cmd);
			return ESP_FAIL;
		}

		memcpy(packet + (i * (64 + 1) + 8 * 2), rmt_grayscale,
				sizeof(*packet) * 8 * 2 * 3);
		free(rmt_grayscale);

		if (i == (count_leds - 1))
			packet[(i+1) * (64 + 1) - 1] = rmt_GSLAT;
		else
			packet[(i+1) * (64 + 1) - 1] = rmt_EOS;
	}

	free(rmt_write_cmd);

	return rmt_write_items(h->rmt_ch, packet, (64 + 1) * count_leds, true);
}

esp_err_t tlc59731_release(tlc59731_handle_t **h)
{
	esp_err_t ret;

	if (!h || !(*h))
		return ESP_ERR_INVALID_ARG;

	ret = rmt_driver_uninstall((*h)->rmt_ch);
	free(*h);
	*h = NULL;

	return ret;
}
