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

#ifndef __SX1278_H__
#define __SX1278_H__

/* SX127x Registers addresses */
#define SX127X_REG_FIFO				0x00
#define SX127X_REG_OP_MODE			0x01
#define SX127X_REG_FRF_MSB			0x06
#define SX127X_REG_FRF_MID			0x07
#define SX127X_REG_FRF_LSB			0x08
#define SX127X_REG_PA_CONFIG			0x09
#define SX127X_REG_PA_RAMP			0x0A
#define SX127X_REG_OCP				0x0B
#define SX127X_REG_LNA				0x0C
#define SX127X_REG_FIFO_ADDR_PTR		0x0D
#define SX127X_REG_FIFO_TX_BASE_ADDR		0x0E
#define SX127X_REG_FIFO_RX_BASE_ADDR		0x0F
#define SX127X_REG_FIFO_RX_CURRENT_ADDR		0x10
#define SX127X_REG_IRQ_FLAGS_MASK		0x11
#define SX127X_REG_IRQ_FLAGS			0x12
#define SX127X_REG_RX_NB_BYTES			0x13
#define SX127X_REG_RX_HEADER_CNT_VALUE_MSB	0x14
#define SX127X_REG_RX_HEADER_CNT_VALUE_LSB	0x15
#define SX127X_REG_RX_PACKET_CNT_VALUE_MSB	0x16
#define SX127X_REG_RX_PACKET_CNT_VALUE_LSB	0x17
#define SX127X_REG_MODEM_STAT			0x18
#define SX127X_REG_PKT_SNR_VALUE		0x19
#define SX127X_REG_PKT_RSSI_VALUE		0x1A
#define SX127X_REG_RSSI_VALUE			0x1B
#define SX127X_REG_HOP_CHANNEL			0x1C
#define SX127X_REG_MODEM_CONFIG1		0x1D
#define SX127X_REG_MODEM_CONFIG2		0x1E
#define SX127X_REG_SYMB_TIMEOUT_LSB		0x1F
#define SX127X_REG_PREAMBLE_MSB			0x20
#define SX127X_REG_PREAMBLE_LSB			0x21
#define SX127X_REG_PAYLOAD_LENGTH		0x22
#define SX127X_REG_MAX_PAYLOAD_LENGTH		0x23
#define SX127X_REG_HOP_PERIOD			0x24
#define SX127X_REG_FIFO_RX_BYTE_ADDR		0x25
#define SX127X_REG_MODEM_CONFIG3		0x26
#define SX127X_REG_FEI_MSB			0x28
#define SX127X_REG_FEI_MID			0x29
#define SX127X_REG_FEI_LSB			0x2A
#define SX127X_REG_RSSI_WIDEBAND		0x2C
#define SX127X_REG_DETECT_OPTIMIZE		0x31
#define SX127X_REG_INVERT_IRQ			0x33
#define SX127X_REG_DETECTION_THRESHOLD		0x37
#define SX127X_REG_SYNC_WORD			0x39
#define SX127X_REG_VERSION			0x42
#define SX127X_REG_TCXO				0x4B
#define SX127X_REG_PA_DAC			0x4D
#define SX127X_REG_FORMER_TEMP			0x5B
#define SX127X_REG_AGC_REF			0x61
#define SX127X_REG_AGC_THRESH1			0x62
#define SX127X_REG_AGC_THRESH2			0x63
#define SX127X_REG_AGC_THRESH3			0x64
#define SX127X_REG_PLL				0x70

/* SX127X's operating states in LoRa mode */
#define SX127X_SLEEP_MODE			0x00
#define SX127X_STANDBY_MODE			0x01
#define SX127X_FSTX_MODE			0x02
#define SX127X_TX_MODE				0x03
#define SX127X_FSRX_MODE			0x04
#define SX127X_RXCONTINUOUS_MODE		0x05
#define SX127X_RXSINGLE_MODE			0x06
#define SX127X_CAD_MODE				0x07

/* SX127X's IRQ flags in LoRa mode */
#define SX127X_FLAG_RXTIMEOUT			0x80
#define	SX127X_FLAG_RXDONE			0x40
#define SX127X_FLAG_PAYLOADCRCERROR		0x20
#define SX127X_FLAG_VALIDHEADER			0x10
#define SX127X_FLAG_TXDONE			0x08
#define SX127X_FLAG_CADDONE			0x04
#define SX127X_FLAG_FHSSCHANGECHANNEL		0x02
#define SX127X_FLAG_CADDETECTED			0x01

/* SX127X's IRQ flags' mask for output pins in LoRa mode */
#define SX127X_FLAGMASK_RXTIMEOUT		0x80
#define	SX127X_FLAGMASK_RXDONE			0x40
#define SX127X_FLAGMASK_PAYLOADCRCERROR		0x20
#define SX127X_FLAGMASK_VALIDHEADER		0x10
#define SX127X_FLAGMASK_TXDONE			0x08
#define SX127X_FLAGMASK_CADDONE			0x04
#define SX127X_FLAGMASK_FHSSCHANGECHANNEL	0x02
#define SX127X_FLAGMASK_CADDETECTED		0x01

int
init_sx127X(struct spi_device *spi);

ssize_t
sx127X_sync(struct spi_device *spi, struct spi_message *);

int
sx127X_read_reg(struct spi_device *spi, uint8_t adr, void *buf, size_t len);

int
sx127X_write_reg(struct spi_device *spi, uint8_t adr, void *buf, size_t len);

void
sx127X_startLoRaMode(struct spi_device *spi);

uint8_t 
sx127X_getMode(struct spi_device *spi);

void
sx127X_setState(struct spi_device *spi, uint8_t st);

uint8_t
sx127X_getState(struct spi_device *spi);

void
sx127X_setLoRaFreq(struct spi_device *spi, uint32_t fr);

uint32_t
sx127X_getLoRaFreq(struct spi_device *spi);

void
sx127X_setLoRaPower(struct spi_device *spi, int32_t pout);

int32_t
sx127X_getLoRaPower(struct spi_device *spi);

int
sx127X_readVersion(struct spi_device *spi, char *vstr, size_t len);

uint8_t
sx127X_getLoRaAllFlag(struct spi_device *spi);

#define sx127X_getLoRaFlag(spi, f)	(sx127X_getLoRaAllFlag(spi) & (f))

void
sx127X_clearLoRaFlag(struct spi_device *spi, uint8_t f);

#define sx127X_clearLoRaAllFlag(spi)	sx127X_clearLoRaFlag(spi, 0xFF)

void
sx127X_setLoRaSPRFactor(struct spi_device *spi, uint32_t chips);

uint32_t
sx127X_getLoRaSPRFactor(struct spi_device *spi);

void
sx127X_setLoRaBW(struct spi_device *spi, uint32_t bw);

uint32_t
sx127X_getLoRaBW(struct spi_device *spi);

void
sx127X_setLoRaCR(struct spi_device *spi, uint8_t cr);

uint8_t
sx127X_getLoRaCR(struct spi_device *spi);

void
sx127X_setLoRaImplicit(struct spi_device *spi, uint8_t yesno);

void
sx127X_setLoRaRXTimeout(struct spi_device *spi, uint32_t ms);

uint32_t
sx127X_getLoRaRXTimeout(struct spi_device *spi);

void
sx127X_setLoRaMaxRXBuff(struct spi_device *spi, uint8_t len);

ssize_t
sx127X_readLoRaData(struct spi_device *spi, uint8_t *buf, size_t len);

ssize_t
sx127X_sendLoRaData(struct spi_device *spi, uint8_t *buf, size_t len);

int32_t
sx127X_getLoRaLastPacketRSSI(struct spi_device *spi);

uint32_t
sx127X_getLoRaLastPacketSNR(struct spi_device *spi);

int32_t
sx127X_getLoRaRSSI(struct spi_device *spi);

void
sx127X_setLoRaPreambleLen(struct spi_device *spi, uint32_t len);

uint32_t
sx127X_getLoRaPreambleLen(struct spi_device *spi);

void
sx127X_setLoRaCRC(struct spi_device *spi, uint8_t yesno);

void
sx127X_setBoost(struct spi_device *spi, uint8_t yesno);

#endif
