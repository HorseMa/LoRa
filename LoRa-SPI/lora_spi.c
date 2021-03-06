/*-
 * Copyright (c) 2017 Jian-Hong, Pan <starnight@g.ncu.edu.tw>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    similar to the "NO WARRANTY" disclaimer below ("Disclaimer") and any
 *    redistribution must be conditioned upon including a substantially
 *    similar Disclaimer requirement for further binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF NONINFRINGEMENT, MERCHANTIBILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGES.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/compat.h>
#include <linux/acpi.h>
#include <linux/spi/spi.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/errno.h>
#include <linux/delay.h>

#include "lora_spi.h"
#include "sx1278.h"

#define __DRIVER_NAME		"lora-spi"
#ifndef N_LORASPI_MINORS
#define N_LORASPI_MINORS	8
#endif

static DECLARE_BITMAP(minors, N_LORASPI_MINORS);

static DEFINE_MUTEX(minors_lock);

/**
 * loraspi_read - Read from the LoRa device's communication
 * @lrdata:	LoRa device
 * @arg:	the buffer going to hold the read data in user space
 * @size:	the length of the buffer in bytes
 *
 * Return:	Read how many bytes actually, negative number for error
 */
static ssize_t
loraspi_read(struct lora_struct *lrdata, const char __user *buf, size_t size)
{
	struct spi_device *spi;
	ssize_t status;
	int c = 0;
	uint8_t adr;
	uint8_t flag;
	uint8_t st;
	uint32_t timeout;

	spi = lrdata->lora_device;
	dev_dbg(&(spi->dev), "Read %zu bytes into user space\n", size);

	mutex_lock(&(lrdata->buf_lock));
	/* Get chip's current state. */
	st = sx127X_getState(spi);

	/*  Prepare and set the chip to RX continuous mode, if it is not. */
	if (st != SX127X_RXCONTINUOUS_MODE) {
		/* Set chip to standby state. */
		dev_dbg(&(spi->dev), "Going to set standby state\n");
		sx127X_setState(spi, SX127X_STANDBY_MODE);

		/* Set chip FIFO RX base. */
		adr = 0x00;
		dev_dbg(&(spi->dev), "Going to set RX base address\n");
		sx127X_write_reg(spi, SX127X_REG_FIFO_RX_BASE_ADDR, &adr, 1);

		/* Clear all of the IRQ flags. */
		sx127X_clearLoRaAllFlag(spi);
		/* Set chip to RX continuous state waiting for receiving. */
		sx127X_setState(spi, SX127X_RXCONTINUOUS_MODE);
	}

	/* Wait and check there is any packet received ready. */
	for (timeout = 0; timeout < 250; timeout++) {
		flag = sx127X_getLoRaFlag(spi,
					SX127X_FLAG_RXTIMEOUT |
					SX127X_FLAG_RXDONE |
					SX127X_FLAG_PAYLOADCRCERROR);
		if (flag == 0)
			msleep(20);
		else
			break;
	}

	/* If there is nothing or received timeout. */
	if ((flag == 0) || (flag & SX127X_FLAG_RXTIMEOUT)) {
		c = -1;
	}
	/* If there is a packet, but the payload is CRC error. */
	if (sx127X_getLoRaFlag(spi, SX127X_FLAG_PAYLOADCRCERROR)) {
		c = -2;
	}

	/* There is a ready packet in the chip's FIFO. */
	if (c == 0) {
		memset(lrdata->rx_buf, 0, lrdata->bufmaxlen);
		size = (lrdata->bufmaxlen < size) ? lrdata->bufmaxlen : size;
		/* Read from chip to LoRa data RX buffer. */
		c = sx127X_readLoRaData(spi, lrdata->rx_buf, size);
		/* Copy from LoRa data RX buffer to user space. */
		if (c > 0)
			status = copy_to_user((void *)buf, lrdata->rx_buf, c);
	}

	/* Clear all of the IRQ flags. */
	sx127X_clearLoRaAllFlag(spi);

	mutex_unlock(&(lrdata->buf_lock));

	return c;
}

/**
 * loraspi_write - Write to the LoRa device's communication
 * @lrdata:	LoRa device
 * @arg:	the buffer holding the data going to be written in user space
 * @size:	the length of the buffer in bytes
 *
 * Return:	Write how many bytes actually, negative number for error
 */
static ssize_t
loraspi_write(struct lora_struct *lrdata, const char __user *buf, size_t size)
{
	struct spi_device *spi;
	ssize_t status;
	int c;
	uint8_t adr;
	uint8_t flag;
	uint32_t timeout;

	spi = lrdata->lora_device;
	dev_dbg(&(spi->dev), "Write %zu bytes from user space\n", size);

	mutex_lock(&(lrdata->buf_lock));
	memset(lrdata->tx_buf, 0, lrdata->bufmaxlen);
	status = copy_from_user(lrdata->tx_buf, buf, size);

	if (status >= size)
		return 0;

	lrdata->tx_buflen = size - status;

	/* Set chip to standby state. */
	dev_dbg(&(spi->dev), "Going to set standby state\n");
	sx127X_setState(spi, SX127X_STANDBY_MODE);

	/* Set chip FIFO TX base. */
	adr = 0x80;
	dev_dbg(&(spi->dev), "Going to set TX base address\n");
	sx127X_write_reg(spi, SX127X_REG_FIFO_TX_BASE_ADDR, &adr, 1);

	/* Write to SPI chip synchronously to fill the FIFO of the chip. */
	c = sx127X_sendLoRaData(spi, lrdata->tx_buf, lrdata->tx_buflen);

	/* Clear LoRa IRQ TX flag. */
	sx127X_clearLoRaFlag(spi, SX127X_FLAG_TXDONE);

	if (c > 0) {
		/* Set chip to TX state to send the data in FIFO to RF. */
		dev_dbg(&(spi->dev), "Set TX state\n");
		sx127X_setState(spi, SX127X_TX_MODE);

		timeout = (c + sx127X_getLoRaPreambleLen(spi) + 1) + 2;
		dev_dbg(&(spi->dev), "The time out is %u ms", timeout * 20);

		/* Wait until TX is finished by checking the TX flag. */
		for (flag = 0; timeout > 0; timeout--) {
			flag = sx127X_getLoRaFlag(spi, SX127X_FLAG_TXDONE);
			if (flag != 0) {
				dev_dbg(&(spi->dev), "Wait TX is finished\n");
				break;
			}

			if (timeout == 1) {
				c = 0;
				dev_dbg(&(spi->dev), "Wait TX is time out\n");
			}
			else {
				msleep(20);
			}
		}
	}

	/* Set chip to RX continuous state. */
	dev_dbg(&(spi->dev), "Set back to RX continuous state\n");
	sx127X_setState(spi, SX127X_STANDBY_MODE);
	sx127X_setState(spi, SX127X_RXCONTINUOUS_MODE);

	lrdata->tx_buflen = 0;

	mutex_unlock(&(lrdata->buf_lock));

	return c;
}

/**
 * loraspi_setstate - Set the state of the LoRa device
 * @lrdata:	LoRa device
 * @arg:	the buffer holding the state value in user space
 *
 * Return:	0 / other values for success / error
 */
static long
loraspi_setstate(struct lora_struct *lrdata, void __user *arg)
{
	struct spi_device *spi;
	int status;
	uint32_t st32;
	uint8_t st;

	spi = lrdata->lora_device;
	status = copy_from_user(&st32, arg, sizeof(uint32_t));
	switch (st32) {
	case LORA_STATE_SLEEP:
		st = SX127X_SLEEP_MODE;		break;
	case LORA_STATE_STANDBY:
		st = SX127X_STANDBY_MODE;	break;
	case LORA_STATE_TX:
		st = SX127X_TX_MODE;		break;
	case LORA_STATE_RX:
		st = SX127X_RXCONTINUOUS_MODE;	break;
	case LORA_STATE_CAD:
		st = SX127X_CAD_MODE;		break;
	default:
		st = SX127X_STANDBY_MODE;
	}

	mutex_lock(&(lrdata->buf_lock));
	sx127X_setState(spi, st);
	mutex_unlock(&(lrdata->buf_lock));

	return 0;
}

/**
 * loraspi_getstate - Get the state of the LoRa device
 * @lrdata:	LoRa device
 * @arg:	the buffer going to hold the state value in user space
 *
 * Return:	0 / other values for success / error
 */
static long
loraspi_getstate(struct lora_struct *lrdata, void __user *arg)
{
	struct spi_device *spi;
	int status;
	uint32_t st32;
	uint8_t st;

	spi = lrdata->lora_device;

	mutex_lock(&(lrdata->buf_lock));
	st = sx127X_getState(spi);
	mutex_unlock(&(lrdata->buf_lock));

	st32 = st;
	switch (st) {
	case SX127X_SLEEP_MODE:
		st32 = LORA_STATE_SLEEP;	break;
	case SX127X_STANDBY_MODE:
		st32 = LORA_STATE_STANDBY;	break;
	case SX127X_FSTX_MODE:
	case SX127X_TX_MODE:
		st32 = LORA_STATE_TX;		break;
	case SX127X_FSRX_MODE:
	case SX127X_RXSINGLE_MODE:
	case SX127X_RXCONTINUOUS_MODE:
		st32 = LORA_STATE_RX;		break;
	case SX127X_CAD_MODE:
		st32 = LORA_STATE_CAD;		break;
	default:
		st32 = LORA_STATE_SLEEP;
	}
	status = copy_to_user(arg, &st32, sizeof(uint32_t));

	return 0;
}

/**
 * loraspi_setfreq - Set the carrier frequency
 * @lrdata:	LoRa device
 * @arg:	the buffer holding the carrier frequency in user space
 *
 * Return:	0 / other values for success / error
 */
static long
loraspi_setfreq(struct lora_struct *lrdata, void __user *arg)
{
	struct spi_device *spi;
	int status;
	uint32_t freq;

	spi = lrdata->lora_device;
	status = copy_from_user(&freq, arg, sizeof(uint32_t));
	dev_dbg(&(spi->dev), "Set frequency %u Hz from user space\n", freq);

	mutex_lock(&(lrdata->buf_lock));
	sx127X_setLoRaFreq(spi, freq);
	mutex_unlock(&(lrdata->buf_lock));

	return 0;
}

/**
 * loraspi_getfreq - Get the carrier frequency
 * @lrdata:	LoRa device
 * @arg:	the buffer going to hold the carrier frequency in user space
 *
 * Return:	0 / other values for success / error
 */
static long
loraspi_getfreq(struct lora_struct *lrdata, void __user *arg)
{
	struct spi_device *spi;
	int status;
	uint32_t freq;

	spi = lrdata->lora_device;
	dev_dbg(&(spi->dev), "Get frequency to user space\n");

	mutex_lock(&(lrdata->buf_lock));
	freq = sx127X_getLoRaFreq(spi);
	mutex_unlock(&(lrdata->buf_lock));
	dev_dbg(&(spi->dev), "The carrier freq is %u Hz\n", freq);

	status = copy_to_user(arg, &freq, sizeof(uint32_t));

	return 0;
}

/**
 * loraspi_setpower - Set the PA power
 * @lrdata:	LoRa device
 * @arg:	the buffer holding the PA output value in user space
 *
 * Return:	0 / other values for success / error
 */
static long
loraspi_setpower(struct lora_struct *lrdata, void __user *arg)
{
	struct spi_device *spi;
	int status;
	int32_t dbm;

	spi = lrdata->lora_device;
	status = copy_from_user(&dbm, arg, sizeof(uint32_t));

#define LORA_MAX_POWER	(17)
#define LORA_MIN_POWER	(-2)
	if (dbm > LORA_MAX_POWER)
		dbm = LORA_MAX_POWER;
	else if (dbm < LORA_MIN_POWER)
		dbm = LORA_MIN_POWER;

	mutex_lock(&(lrdata->buf_lock));
	sx127X_setLoRaPower(spi, dbm);
	mutex_unlock(&(lrdata->buf_lock));

	return 0;
}

/**
 * loraspi_getpower -  Get the PA power
 * @lrdata:	LoRa device
 * @arg:	the buffer going to hold the PA output value in user space
 *
 * Return:	0 / other values for success / error
 */
static long
loraspi_getpower(struct lora_struct *lrdata, void __user *arg)
{
	struct spi_device *spi;
	int status;
	int32_t dbm;

	spi = lrdata->lora_device;

	mutex_lock(&(lrdata->buf_lock));
	dbm = sx127X_getLoRaPower(spi);
	mutex_unlock(&(lrdata->buf_lock));

	status = copy_to_user(arg, &dbm, sizeof(uint32_t));

	return 0;
}

/**
 * loraspi_setsprfactor - Set the RF spreading factor
 * @lrdata:	LoRa device
 * @arg:	the buffer holding the spreading factor in user space
 *
 * Return:	0 / other values for success / error
 */
static long
loraspi_setsprfactor(struct lora_struct *lrdata, void __user *arg)
{
	struct spi_device *spi;
	int status;
	uint32_t sprf;

	spi = lrdata->lora_device;
	status = copy_from_user(&sprf, arg, sizeof(uint32_t));

	mutex_lock(&(lrdata->buf_lock));
	sx127X_setLoRaSPRFactor(spi, sprf);
	mutex_unlock(&(lrdata->buf_lock));

	return 0;
}

/**
 * loraspi_getsprfactor - Get the RF spreading factor
 * @lrdata:	LoRa device
 * @arg:	the buffer going to hold the spreading factor in user space
 *
 * Return:	0 / other values for success / error
 */
static long
loraspi_getsprfactor(struct lora_struct *lrdata, void __user *arg)
{
	struct spi_device *spi;
	int status;
	uint32_t sprf;

	spi = lrdata->lora_device;

	mutex_lock(&(lrdata->buf_lock));
	sprf = sx127X_getLoRaSPRFactor(spi);
	mutex_unlock(&(lrdata->buf_lock));

	status = copy_to_user(arg, &sprf, sizeof(uint32_t));

	return 0;
}

/**
 * loraspi_setbandwidth - Set the RF bandwith
 * @lrdata:	LoRa device
 * @arg:	the buffer holding the RF bandwith value in user space
 *
 * Return:	0 / other values for success / error
 */
static long
loraspi_setbandwidth(struct lora_struct *lrdata, void __user *arg)
{
	struct spi_device *spi;
	int status;
	uint32_t bw;

	spi = lrdata->lora_device;
	status = copy_from_user(&bw, arg, sizeof(uint32_t));

	mutex_lock(&(lrdata->buf_lock));
	sx127X_setLoRaBW(spi, bw);
	mutex_unlock(&(lrdata->buf_lock));

	return 0;
}

/**
 * loraspi_getbandwidth - Get the RF bandwith
 * @lrdata:	LoRa device
 * @arg:	the buffer going to hold the RF bandwith value in user space
 *
 * Return:	0 / other values for success / error
 */
static long
loraspi_getbandwidth(struct lora_struct *lrdata, void __user *arg)
{
	struct spi_device *spi;
	int status;
	uint32_t bw;

	spi = lrdata->lora_device;

	mutex_lock(&(lrdata->buf_lock));
	bw = sx127X_getLoRaBW(spi);
	mutex_unlock(&(lrdata->buf_lock));

	status = copy_to_user(arg, &bw, sizeof(uint32_t));

	return 0;
}

/**
 * loraspi_getrssi - Get current RSSI
 * @lrdata:	LoRa device
 * @arg:	the buffer going to hold the RSSI value in user space
 *
 * Return:	0 / other values for success / error
 */
static long
loraspi_getrssi(struct lora_struct *lrdata, void __user *arg)
{
	struct spi_device *spi;
	int status;
	int32_t rssi;

	spi = lrdata->lora_device;

	mutex_lock(&(lrdata->buf_lock));
	rssi = sx127X_getLoRaRSSI(spi);
	mutex_unlock(&(lrdata->buf_lock));

	status = copy_to_user(arg, &rssi, sizeof(int32_t));

	return 0;
}

/**
 * loraspi_getsnr - Get last packet's SNR
 * @lrdata:	LoRa device
 * @arg:	the buffer going to hold the SNR value in user space
 *
 * Return:	0 / other values for success / error
 */
static long
loraspi_getsnr(struct lora_struct *lrdata, void __user *arg)
{
	struct spi_device *spi;
	int status;
	uint32_t snr;

	spi = lrdata->lora_device;

	mutex_lock(&(lrdata->buf_lock));
	snr = sx127X_getLoRaLastPacketSNR(spi);
	mutex_unlock(&(lrdata->buf_lock));

	status = copy_to_user(arg, &snr, sizeof(uint32_t));

	return 0;
}

/**
 * loraspi_ready2write - Is ready to be written
 * @lrdata:	LoRa device
 *
 * Return:	1 / 0 for ready / not ready
 */
static long
loraspi_ready2write(struct lora_struct *lrdata)
{
	long ret;

	/* Mutex is not lock, than it is not writing. */
	ret = mutex_is_locked(&(lrdata->buf_lock)) ? 0 : 1;

	return ret;
}

/**
 * loraspi_ready2read - Is ready to be read
 * @lrdata:	LoRa device
 *
 * Return:	1 / 0 for ready / not ready
 */
static long
loraspi_ready2read(struct lora_struct *lrdata)
{
	struct spi_device *spi;
	long ret;

	spi = lrdata->lora_device;

	ret = 0;
	/* Mutex is not lock, than it is not in reading file operation. */
	if (!mutex_is_locked(&(lrdata->buf_lock))) {
		/* Check the chip have recieved full data. */
		mutex_lock(&(lrdata->buf_lock));
		ret = sx127X_getLoRaFlag(spi, SX127X_FLAG_RXDONE) != 0;
		mutex_unlock(&(lrdata->buf_lock));
	}

	return ret;
}

struct lora_driver lr_driver = {
	.name = __DRIVER_NAME,
	.num = N_LORASPI_MINORS,
	.owner = THIS_MODULE,
};

struct lora_operations lrops = {
	.read = loraspi_read,
	.write = loraspi_write,
	.setState = loraspi_setstate,
	.getState = loraspi_getstate,
	.setFreq = loraspi_setfreq,
	.getFreq = loraspi_getfreq,
	.setPower = loraspi_setpower,
	.getPower = loraspi_getpower,
	.setSPRFactor = loraspi_setsprfactor,
	.getSPRFactor = loraspi_getsprfactor,
	.setBW = loraspi_setbandwidth,
	.getBW = loraspi_getbandwidth,
	.getRSSI = loraspi_getrssi,
	.getSNR = loraspi_getsnr,
	.ready2write = loraspi_ready2write,
	.ready2read = loraspi_ready2read,
};

/* The compatible SoC array. */
#ifdef CONFIG_OF
static const struct of_device_id lora_dt_ids[] = {
	{ .compatible = "semtech,sx1276" },
	{ .compatible = "semtech,sx1277" },
	{ .compatible = "semtech,sx1278" },
	{ .compatible = "semtech,sx1279" },
	{ .compatible = "lora-spi" },
	{}, /* Should be terminated with a NULL entry. */
};
MODULE_DEVICE_TABLE(of, lora_dt_ids);
#endif

#ifdef CONFIG_ACPI

/* The compatible ACPI device array. */
#define LORA_ACPI_DUMMY	1
static const struct acpi_device_id lora_acpi_ids[] = {
	{ .id = "lora-spi" },
	{}, /* Should be terminated with a NULL entry. */
};
MODULE_DEVICE_TABLE(acpi, lora_acpi_ids);

/* The callback function of ACPI probes LoRa SPI. */
static void loraspi_probe_acpi(struct spi_device *spi) {
	const struct acpi_device_id *id;

	if (!has_acpi_companion(&(spi->dev)))
		return;

	id = acpi_match_device(lora_acpi_ids, &(spi->dev));
	if (WARN_ON(!id))
		return;

	if (id->driver_data == LORA_ACPI_DUMMY)
		dev_warn(&(spi->dev),
			"Do not use this driver in produciton systems.\n");
}
#else
static void loraspi_probe_acpi(struct spi_device *spi) {};
#endif

/* The compatible SPI device id array. */
static const struct spi_device_id spi_ids[] = {
	{ .name = "lora-spi" },
	{}, /* Should be terminated with a NULL entry. */
};
MODULE_DEVICE_TABLE(spi, spi_ids);

/* The SPI probe callback function. */
static int loraspi_probe(struct spi_device *spi)
{
	struct lora_struct *lrdata;
	struct device *dev;
	unsigned long minor;
	int status;

	dev_info(&(spi->dev), "probe a LoRa SPI device\n");

#ifdef CONFIG_OF
	if (spi->dev.of_node && !of_match_device(lora_dt_ids, &(spi->dev))) {
		dev_err(&(spi->dev), "buggy DT: LoRa listed directly in DT\n");
		WARN_ON(spi->dev.of_node &&
			!of_match_device(lora_dt_ids, (&spi->dev)));
	}
#endif

	loraspi_probe_acpi(spi);

	/* Allocate lora device's data. */
	lrdata = kzalloc(sizeof(struct lora_struct), GFP_KERNEL);
	if (!lrdata)
		return -ENOMEM;

	/* Initial the lora device's data. */
	lrdata->lora_device = spi;
	lrdata->ops = &lrops;
	mutex_init(&(lrdata->buf_lock));
	mutex_lock(&minors_lock);
	minor = find_first_zero_bit(minors, N_LORASPI_MINORS);
	if (minor < N_LORASPI_MINORS) {
		set_bit(minor, minors);
		lrdata->devt = MKDEV(lr_driver.major, minor);
		dev = device_create(lr_driver.lora_class,
				&(spi->dev),
				lrdata->devt,
				lrdata,
				"loraSPI%d.%d",
				spi->master->bus_num, spi->chip_select);
		/* Set the SPI device's driver data for later use.  */
		spi_set_drvdata(spi, lrdata);
		lora_device_add(lrdata);
		status = PTR_ERR_OR_ZERO(dev);
	}
	else {
		/* No more lora device available. */
		kfree(lrdata);
		status = -ENODEV;
	}
	
	/* Initial the SX127X chip. */
	init_sx127X(spi);
	
	mutex_unlock(&minors_lock);

	return status;
}

/* The SPI remove callback function. */
static int loraspi_remove(struct spi_device *spi)
{
	struct lora_struct *lrdata;
	
	dev_info(&(spi->dev), "remove a LoRa SPI device");

	lrdata = spi_get_drvdata(spi);

	/* Clear the lora device's data. */
	lrdata->lora_device = NULL;
	/* No more operations to the lora device from user space. */
	lora_device_remove(lrdata);
	mutex_lock(&minors_lock);
	device_destroy(lr_driver.lora_class, lrdata->devt);
	clear_bit(MINOR(lrdata->devt), minors);
	/* Set the SX127X chip to sleep. */
	sx127X_setState(spi, SX127X_SLEEP_MODE);
	mutex_unlock(&minors_lock);

	/* Free the memory of the lora device.  */
	kfree(lrdata);
	
	return 0;
}

/* The SPI driver which acts as a protocol driver in this kernel module. */
static struct spi_driver lora_spi_driver = {
	.driver = {
		.name = __DRIVER_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = lora_dt_ids,
#endif
#ifdef CONFIG_ACPI
		.acpi_match_table = ACPI_PTR(lora_acpi_ids),
#endif
	},
	.probe = loraspi_probe,
	.remove = loraspi_remove,
	.id_table = spi_ids,
};


/* LoRa-SPI kernel module's initial function. */
static int loraspi_init(void)
{
	int status;
	
	pr_debug("lora-spi: init SX1278 compatible kernel module\n");
	
	/* Register a kind of LoRa driver. */
	lora_register_driver(&lr_driver);

	/* Register LoRa SPI driver as an SPI driver. */
	status = spi_register_driver(&lora_spi_driver);

	return status;
}

/* LoRa-SPI kernel module's exit function. */
static void loraspi_exit(void)
{
	pr_debug("lora-spi: exit\n");

	/* Unregister the LoRa SPI driver. */
	spi_unregister_driver(&lora_spi_driver);
	/* Unregister the lora driver. */
	lora_unregister_driver(&lr_driver);
}

module_init(loraspi_init);
module_exit(loraspi_exit);

MODULE_AUTHOR("Jian-Hong Pan, <starnight@g.ncu.edu.tw>");
MODULE_DESCRIPTION("LoRa device kernel module with SPI interface");
MODULE_LICENSE("Dual BSD/GPL");
