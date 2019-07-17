/*
 * TLC59731.c
 *
 *  Created on: 14. sij 2019.
 *      Author: Admin
 */

#include "TLC59731.h"

static rmt_config_t tlc59731_getRMTConfig()
{
	rmt_config_t rmtConfig;
	rmtConfig.rmt_mode = RMT_MODE_TX;
	rmtConfig.channel = RMT_CHANNEL_0;
	rmtConfig.gpio_num = TLC59731_PIN;
	rmtConfig.mem_block_num = 1;
	rmtConfig.tx_config.loop_en = 0;
	rmtConfig.tx_config.carrier_en = 0;
	rmtConfig.tx_config.idle_output_en = 1;
	rmtConfig.tx_config.idle_level = 0;
	/*
	 * The RMT source clock is typically APB CLK of 80MHz, thus
	 * 80MHz / 160 = 500KHz, and thus 1 tick equals
	 * 1 / 500.000Hz = 2us
	 */
	rmtConfig.clk_div = 160;
	
	return rmtConfig;
}

static rmt_item32_t tlc59731_getEOSInRmtItem()
{
	rmt_item32_t rmtEOS =
			{
			{
			{ INTERVAL_US(TLC59731_T_CYCLE*2), 0, INTERVAL_US(
					TLC59731_T_CYCLE*2), 0 } } };

	return rmtEOS;
}

static rmt_item32_t tlc59731_getGSLATInRmtItem()
{
	rmt_item32_t rmtGSLAT =
			{
			{
			{ INTERVAL_US(TLC59731_T_CYCLE*4), 0, INTERVAL_US(
					TLC59731_T_CYCLE*4), 0 } } };

	return rmtGSLAT;
}

static rmt_item32_t* tlc59731_getBitInRmtItem(uint8_t bit)
{
	rmt_item32_t* rmtBit = (rmt_item32_t*) malloc(sizeof(rmt_item32_t) * 2);

	rmtBit[0].level0 = 1;
	rmtBit[0].duration0 = INTERVAL_US(TLC59731_T_CYCLE/5);
	rmtBit[0].level1 = 0;
	rmtBit[0].duration1 = INTERVAL_US(TLC59731_T_CYCLE/5);

	if (bit & 0x80)
	{
		rmtBit[1].level0 = 1;
	}
	else
	{
		rmtBit[1].level0 = 0;
	}
	rmtBit[1].duration0 = INTERVAL_US(TLC59731_T_CYCLE/5);
	rmtBit[1].level1 = 0;
	rmtBit[1].duration1 = INTERVAL_US((TLC59731_T_CYCLE/5)*3);

	return rmtBit;
}

static rmt_item32_t* tlc59731_getByteInRmtItem(uint8_t toConvert)
{
	rmt_item32_t* rmtWriteCmd = (rmt_item32_t*) malloc(
			sizeof(rmt_item32_t) * 2 * 8);

	if (rmtWriteCmd != NULL)
	{
		for (uint8_t i = 0; i < 8; i++)
		{
			uint8_t tmp = toConvert << i;
			rmt_item32_t* rmtBit = tlc59731_getBitInRmtItem(tmp & 0x80);
			memcpy(rmtWriteCmd + (i * 2), rmtBit, sizeof(rmt_item32_t) * 2);

			free(rmtBit);
		}
	}

	return rmtWriteCmd;
}

static rmt_item32_t* tlc59731_getWriteCmdInRmtItem()
{
	uint8_t writeCmd = 0x3A;

	return tlc59731_getByteInRmtItem(writeCmd);
}

static rmt_item32_t* tlc59731_getGrayscaleInRmtItem(uint8_t gs[3])
{
	rmt_item32_t* rmtGrayscale = (rmt_item32_t*) malloc(
			sizeof(rmt_item32_t) * 3 * 8 * 2);

	if (rmtGrayscale != NULL)
	{
		for (uint8_t i = 0; i < 3; i++)
		{
			rmt_item32_t* tmpByte = tlc59731_getByteInRmtItem(gs[i]);
			memcpy(rmtGrayscale + (i * 2) * 8, tmpByte,
					sizeof(rmt_item32_t) * 2 * 8);
			free(tmpByte);
		}
	}

	return rmtGrayscale;
}

void tlc59731_init()
{
	gpio_set_direction(TLC59731_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(TLC59731_PIN, 0);
	
	rmt_config_t rmtConfig = tlc59731_getRMTConfig();

	rmt_config(&rmtConfig);
	rmt_driver_install(rmtConfig.channel, 0, 0);
}

void tlc59731_setGrayscale(ledRGB* gs, uint8_t countLEDs)
{
	rmt_item32_t packet[(64 + 1) * countLEDs];

	rmt_item32_t* rmtWriteCmd = tlc59731_getWriteCmdInRmtItem();
	rmt_item32_t rmtEOS = tlc59731_getEOSInRmtItem();
	rmt_item32_t rmtGSLAT = tlc59731_getGSLATInRmtItem();

	for (uint8_t i = 0; i < countLEDs; i++)
	{
		memcpy(packet + (i * (64 + 1)), rmtWriteCmd,
				sizeof(rmt_item32_t) * 8 * 2);

		uint8_t tmpGS[3];
		for (uint8_t j = 0; j < 3; j++)
		{
			switch (j)
			{
			case 0:
				tmpGS[j] = gs[i].r;
				break;
			case 1:
				tmpGS[j] = gs[i].g;
				break;
			case 2:
				tmpGS[j] = gs[i].b;
				break;
			}
		}

		rmt_item32_t* rmtGrayscale = tlc59731_getGrayscaleInRmtItem(tmpGS);
		memcpy(packet + (i * (64 + 1) + 8 * 2), rmtGrayscale,
				sizeof(rmt_item32_t) * 8 * 2 * 3);
		free(rmtGrayscale);
		if (i + 1 == countLEDs)
		{
			packet[(i+1) * (64 + 1) - 1] = rmtGSLAT;
		}
		else
		{
			packet[(i+1) * (64 + 1) - 1] = rmtEOS;
		}
	}

	free(rmtWriteCmd);

	rmt_write_items(RMT_CHANNEL_0, packet, (64 + 1) * countLEDs, true);
}

void tlc59731_release()
{
	rmt_config_t rmtConfig = tlc59731_getRMTConfig();
	
	rmt_driver_uninstall(rmtConfig.channel);
}
