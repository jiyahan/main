/*******************************************************************************
 *
 * Synopsys DesignWare Sensor and Control IP Subsystem IO Software Driver and
 * documentation (hereinafter, "Software") is an Unsupported proprietary work
 * of Synopsys, Inc. unless otherwise expressly agreed to in writing between
 * Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 ******************************************************************************/

/*******************************************************************************
 *
 * Modifications Copyright (c) 2015, Intel Corporation. All rights reserved.
 *
 ******************************************************************************/

#include <stddef.h>

#include "i2c_priv.h"
#include "ss_dw_i2c.h"
#include "machine.h"

static void end_data_transfer(i2c_info_pt dev)
{
	uint32_t state;

	WRITE_ARC_REG(I2C_INT_DSB, dev->reg_base + I2C_INTR_MASK);
	state = dev->state;
	dev->state = I2C_STATE_READY;
	if (I2C_STATE_RECEIVE == state) {
		if (_Usually(NULL != dev->rx_cb)) {
			dev->rx_cb(dev->cb_rx_data);
		}
	} else if (I2C_STATE_TRANSMIT == state) {
		if (_Usually(NULL != dev->tx_cb)) {
			dev->tx_cb(dev->cb_tx_data);
		}
	}
}


static void recv_data(i2c_info_pt dev)
{
	uint32_t i, rx_cnt = 0;


	if (_Rarely(!dev->rx_len)) {
		return;
	}

	rx_cnt = READ_ARC_REG(dev->reg_base + I2C_RXFLR);

	if (rx_cnt > dev->rx_len) {
		rx_cnt = dev->rx_len;
	}
	for (i = 0; i < rx_cnt; i++) {
		WRITE_ARC_REG(I2C_POP_DATA, dev->reg_base + I2C_DATA_CMD);
		_nop();
		dev->i2c_read_buff[i] = READ_ARC_REG(
			dev->reg_base + I2C_DATA_CMD);
	}
	dev->i2c_read_buff += i;
	dev->rx_len -= i;
	dev->total_read_bytes += i;
}


void i2c_fill_fifo(i2c_info_pt dev)
{
	uint32_t i, tx_cnt, data;

	if (_Rarely(!dev->rx_tx_len)) {
		return;
	}

	tx_cnt = dev->fifo_depth - READ_ARC_REG(dev->reg_base + I2C_TXFLR);
	if (tx_cnt > dev->rx_tx_len) {
		tx_cnt = dev->rx_tx_len;
	}

	for (i = 0; i < tx_cnt; i++) {
		if (dev->tx_len > 0) { // something to transmit
			data = I2C_PUSH_DATA | dev->i2c_write_buff[i];

			if (dev->tx_len == 1) { // last byte to write
				if (dev->rx_len > 0) // repeated start  if something to read after
					data |= I2C_RESTART_CMD;
				else
					data |= I2C_STOP_CMD;
			}
			dev->tx_len -= 1;
			dev->total_write_bytes += 1;
		} else { // something to read
			data = I2C_PUSH_DATA | I2C_READ_CMD;
			if (dev->rx_tx_len == 1) // last dummy byte to write
				data |= I2C_STOP_CMD;
		}
		WRITE_ARC_REG(data, dev->reg_base + I2C_DATA_CMD);
		dev->rx_tx_len -= 1;
	}
	dev->i2c_write_buff += i;
}

static void xmit_data(i2c_info_pt dev)
{
	int mask;

	i2c_fill_fifo(dev);
	if (dev->rx_tx_len <= 0) {
		mask = READ_ARC_REG(dev->reg_base + I2C_INTR_MASK);
		mask &= ~(R_TX_EMPTY);
		mask |= R_STOP_DETECTED;
		WRITE_ARC_REG(mask, dev->reg_base + I2C_INTR_MASK);
	}
}


void i2c_mst_err_ISR_proc(i2c_info_pt dev)
{
	uint32_t status = READ_ARC_REG(dev->reg_base + I2C_INTR_STAT);

	if (_Rarely(!status)) {
		return;
	}
	dev->status_code = 0;
	if (status & R_STOP_DETECTED) {
		WRITE_ARC_REG(R_STOP_DETECTED, dev->reg_base + I2C_CLR_INTR);
		//WRITE_ARC_REG( READ_ARC_REG( dev->reg_base + I2C_INTR_MASK) & ~R_STOP_DETECTED, dev->reg_base + I2C_INTR_MASK );
		end_data_transfer(dev);
	}
	if (status & R_TX_ABRT) {
		dev->status_code = status;
		WRITE_ARC_REG(R_TX_ABRT, dev->reg_base + I2C_CLR_INTR);
		WRITE_ARC_REG(I2C_INT_DSB, dev->reg_base + I2C_INTR_MASK);
		dev->state = I2C_STATE_READY;
		if (_Usually(NULL != dev->err_cb)) {
			dev->err_cb(dev->cb_err_data);
		}
	}
	if ((status & R_TX_OVER) || (status & R_RX_OVER)) {
		dev->status_code = status;
		WRITE_ARC_REG(R_TX_OVER | R_RX_OVER,
			      dev->reg_base + I2C_CLR_INTR);
		WRITE_ARC_REG(I2C_INT_DSB, dev->reg_base + I2C_INTR_MASK);
		dev->state = I2C_STATE_READY;
		if (_Usually(NULL != dev->err_cb)) {
			dev->err_cb(dev->cb_err_data);
		}
	}
}


void i2c_mst_rx_avail_ISR_proc(i2c_info_pt dev)
{
#ifndef NDEBUG
	uint32_t status = READ_ARC_REG(dev->reg_base + I2C_INTR_STAT);

	if (_Rarely(!status)) {
		return;
	}

	if (status & R_RX_FULL) {
#endif
	recv_data(dev);
	WRITE_ARC_REG(R_RX_FULL, dev->reg_base + I2C_CLR_INTR);
#ifndef NDEBUG
}
#endif
}


void i2c_mst_tx_req_ISR_proc(i2c_info_pt dev)
{
#ifndef NDEBUG
	uint32_t status = READ_ARC_REG(dev->reg_base + I2C_INTR_STAT);

	if (_Rarely(!status)) {
		return;
	}

	if (status & R_TX_EMPTY) {
#endif
	xmit_data(dev);
	WRITE_ARC_REG(R_TX_EMPTY, dev->reg_base + I2C_CLR_INTR);
#ifndef NDEBUG
}
#endif
}

/* Stop detected ISR - end up here if a STOP condition is asserted on the SDA and SCK signals */
void i2c_mst_stop_detected_ISR_proc(i2c_info_pt dev)
{
	uint32_t status = READ_ARC_REG(dev->reg_base + I2C_INTR_STAT);

	if (status & R_STOP_DETECTED) {
		recv_data(dev);
		end_data_transfer(dev);
	}
	WRITE_ARC_REG(R_STOP_DETECTED, dev->reg_base + I2C_CLR_INTR);
}
