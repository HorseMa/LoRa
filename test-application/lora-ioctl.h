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

#ifndef __LORA_IOCTL_H__
#define __LORA_IOCTL_H__

#include <sys/ioctl.h>

/* I/O control by each command. */
#define LORA_IOC_MAGIC '\x74'

#define LORA_SET_STATE		(_IOW(LORA_IOC_MAGIC,  0, int))
#define LORA_GET_STATE		(_IOR(LORA_IOC_MAGIC,  1, int))
#define LORA_SET_FREQUENCY	(_IOW(LORA_IOC_MAGIC,  2, int))
#define LORA_GET_FREQUENCY	(_IOR(LORA_IOC_MAGIC,  3, int))
#define LORA_SET_POWER		(_IOW(LORA_IOC_MAGIC,  4, int))
#define LORA_GET_POWER		(_IOR(LORA_IOC_MAGIC,  5, int))
#define LORA_SET_SPRFACTOR	(_IOW(LORA_IOC_MAGIC,  6, int))
#define LORA_GET_SPRFACTOR	(_IOR(LORA_IOC_MAGIC,  7, int))
#define LORA_SET_BANDWIDTH	(_IOW(LORA_IOC_MAGIC,  8, int))
#define LORA_GET_BANDWIDTH	(_IOR(LORA_IOC_MAGIC,  9, int))
#define LORA_GET_RSSI		(_IOR(LORA_IOC_MAGIC, 10, int))
#define LORA_GET_SNR		(_IOR(LORA_IOC_MAGIC, 11, int))

/* List the state of the LoRa device. */
#define LORA_STATE_SLEEP	0
#define LORA_STATE_STANDBY	1
#define LORA_STATE_TX		2
#define LORA_STATE_RX		3
#define LORA_STATE_CAD		4

/* Read the device data. */
ssize_t do_read(int fd, char *buf, size_t len);

/* Write data into the device. */
ssize_t do_write(int fd, char *buf, size_t len);

/* Set & get the device's state. */
void set_state(int fd, uint32_t st);
uint32_t get_state(int fd);

/* Set & get the carrier frequency. */
void set_freq(int fd, uint32_t freq);
uint32_t get_freq(int fd);

/* Get current RSSI. */
int32_t get_rssi(int fd);

/* Get last packet SNR. */
uint32_t get_snr(int fd);

/* Set & get output power. */
void set_power(int fd, int32_t power);
int32_t get_power(int fd);

/* Set & get the RF spreading factor. */
void set_sprfactor(int fd, uint32_t sprf);
uint32_t get_sprfactor(int fd);

/* Set & get the RF bandwidth. */
void set_bw(int fd, uint32_t bw);
uint32_t get_bw(int fd);

#endif
