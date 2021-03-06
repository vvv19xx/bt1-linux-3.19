/*
 * Baikal Electronics 10Gb Ethernet driver
 *
 * Based on AMD 10Gb Ethernet driver
 * drivers/net/ethernet/amd/xgbe
 *
 * This file is available to you under your choice of the following two
 * licenses:
 *
 * License 1: GPLv2
 *
 * Copyright (c) 2015 Baikal Electronics JSC.
 * Copyright (c) 2014 Advanced Micro Devices, Inc.
 *
 * This file is free software; you may copy, redistribute and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or (at
 * your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *     The Synopsys DWC ETHER XGMAC Software Driver and documentation
 *     (hereinafter "Software") is an unsupported proprietary work of Synopsys,
 *     Inc. unless otherwise expressly agreed to in writing between Synopsys
 *     and you.
 *
 *     The Software IS NOT an item of Licensed Software or Licensed Product
 *     under any End User Software License Agreement or Agreement for Licensed
 *     Product with Synopsys or any supplement thereto.  Permission is hereby
 *     granted, free of charge, to any person obtaining a copy of this software
 *     annotated with this license and the Software, to deal in the Software
 *     without restriction, including without limitation the rights to use,
 *     copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 *     of the Software, and to permit persons to whom the Software is furnished
 *     to do so, subject to the following conditions:
 *
 *     The above copyright notice and this permission notice shall be included
 *     in all copies or substantial portions of the Software.
 *
 *     THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS"
 *     BASIS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *     TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *     PARTICULAR PURPOSE ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS
 *     BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *     CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *     SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *     INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *     CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *     ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *     THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * License 2: Modified BSD
 *
 * Copyright (c) 2015 Baikal Electronics JSC.
 * Copyright (c) 2014 Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Advanced Micro Devices, Inc. nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *     The Synopsys DWC ETHER XGMAC Software Driver and documentation
 *     (hereinafter "Software") is an unsupported proprietary work of Synopsys,
 *     Inc. unless otherwise expressly agreed to in writing between Synopsys
 *     and you.
 *
 *     The Software IS NOT an item of Licensed Software or Licensed Product
 *     under any End User Software License Agreement or Agreement for Licensed
 *     Product with Synopsys or any supplement thereto.  Permission is hereby
 *     granted, free of charge, to any person obtaining a copy of this software
 *     annotated with this license and the Software, to deal in the Software
 *     without restriction, including without limitation the rights to use,
 *     copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 *     of the Software, and to permit persons to whom the Software is furnished
 *     to do so, subject to the following conditions:
 *
 *     The above copyright notice and this permission notice shall be included
 *     in all copies or substantial portions of the Software.
 *
 *     THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS"
 *     BASIS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *     TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *     PARTICULAR PURPOSE ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS
 *     BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *     CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *     SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *     INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *     CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *     ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
	 *     THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/tcp.h>
#include <linux/if_vlan.h>
#include <net/busy_poll.h>
#include <linux/clk.h>
#include <linux/if_ether.h>
#include <linux/net_tstamp.h>
#include <linux/phy.h>

#include "xgbe.h"
#include "xgbe-common.h"

static int xgbe_one_poll(struct napi_struct *, int);
static int xgbe_all_poll(struct napi_struct *, int);
static void xgbe_set_rx_mode(struct net_device *);

static int xgbe_alloc_channels(struct xgbe_prv_data *pdata)
{
	struct xgbe_channel *channel_mem, *channel;
	struct xgbe_ring *tx_ring, *rx_ring;
	unsigned int count, i;
	int ret = -ENOMEM;

	count = max_t(unsigned int, pdata->tx_ring_count, pdata->rx_ring_count);

	channel_mem = kcalloc(count, sizeof(struct xgbe_channel), GFP_KERNEL);
	
	if (!channel_mem)
		goto err_channel;

	tx_ring = kcalloc(pdata->tx_ring_count, sizeof(struct xgbe_ring),
			  GFP_KERNEL);
	if (!tx_ring)
		goto err_tx_ring;

	rx_ring = kcalloc(pdata->rx_ring_count, sizeof(struct xgbe_ring),
			  GFP_KERNEL);
	if (!rx_ring)
		goto err_rx_ring;

	for (i = 0, channel = channel_mem; i < count; i++, channel++) {
		snprintf(channel->name, sizeof(channel->name), "channel-%d", i);
		channel->pdata = pdata;
		channel->queue_index = i;
		channel->dma_regs = pdata->xgmac_regs + DMA_CH_BASE +
				    (DMA_CH_INC * i);

		if (pdata->per_channel_irq) {
			/* Get the DMA interrupt (offset 1) */
			ret = platform_get_irq(pdata->pdev, i + 1);
			if (ret < 0) {
				netdev_err(pdata->netdev,
					   "platform_get_irq %u failed\n",
					   i + 1);
				goto err_irq;
			}

			channel->dma_irq = ret;
		}

		if (i < pdata->tx_ring_count) {
			spin_lock_init(&tx_ring->lock);
			channel->tx_ring = tx_ring++;
		}

		if (i < pdata->rx_ring_count) {
			spin_lock_init(&rx_ring->lock);
			channel->rx_ring = rx_ring++;
		}

		DBGPR("  %s: queue=%u, dma_regs=%p, dma_irq=%d, tx=%p, rx=%p\n",
		      channel->name, channel->queue_index, channel->dma_regs,
		      channel->dma_irq, channel->tx_ring, channel->rx_ring);
	}

	pdata->channel = channel_mem;
	pdata->channel_count = count;

	return 0;

err_irq:
	kfree(rx_ring);

err_rx_ring:
	kfree(tx_ring);

err_tx_ring:
	kfree(channel_mem);

err_channel:
	return ret;
}

static void xgbe_free_channels(struct xgbe_prv_data *pdata)
{
	if (!pdata->channel)
		return;

	kfree(pdata->channel->rx_ring);
	kfree(pdata->channel->tx_ring);
	kfree(pdata->channel);

	pdata->channel = NULL;
	pdata->channel_count = 0;
}

static inline unsigned int xgbe_tx_avail_desc(struct xgbe_ring *ring)
{
	return (ring->rdesc_count - (ring->cur - ring->dirty));
}

static int xgbe_maybe_stop_tx_queue(struct xgbe_channel *channel,
				    struct xgbe_ring *ring, unsigned int count)
{
	struct xgbe_prv_data *pdata = channel->pdata;

	if (count > xgbe_tx_avail_desc(ring)) {
		DBGPR("  Tx queue stopped, not enough descriptors available\n");
		netif_stop_subqueue(pdata->netdev, channel->queue_index);
		ring->tx.queue_stopped = 1;

		/* If we haven't notified the hardware because of xmit_more
		 * support, tell it now
		 */
		if (ring->tx.xmit_more)
			pdata->hw_if.tx_start_xmit(channel, ring);

		return NETDEV_TX_BUSY;
	}

	return 0;
}

static int xgbe_calc_rx_buf_size(struct net_device *netdev, unsigned int mtu)
{
	unsigned int rx_buf_size;

	if (mtu > XGMAC_JUMBO_PACKET_MTU) {
		netdev_alert(netdev, "MTU exceeds maximum supported value\n");
		return -EINVAL;
	}

	rx_buf_size = mtu + ETH_HLEN + ETH_FCS_LEN + VLAN_HLEN;
	rx_buf_size = clamp_val(rx_buf_size, XGBE_RX_MIN_BUF_SIZE, PAGE_SIZE);

	rx_buf_size = (rx_buf_size + XGBE_RX_BUF_ALIGN - 1) &
		      ~(XGBE_RX_BUF_ALIGN - 1);

	return rx_buf_size;
}

static void xgbe_enable_rx_tx_ints(struct xgbe_prv_data *pdata)
{
	struct xgbe_hw_if *hw_if = &pdata->hw_if;
	struct xgbe_channel *channel;
	enum xgbe_int int_id;
	unsigned int i;

	channel = pdata->channel;
	for (i = 0; i < pdata->channel_count; i++, channel++) {
		if (channel->tx_ring && channel->rx_ring)
			int_id = XGMAC_INT_DMA_CH_SR_TI_RI;
		else if (channel->tx_ring)
			int_id = XGMAC_INT_DMA_CH_SR_TI;
		else if (channel->rx_ring)
			int_id = XGMAC_INT_DMA_CH_SR_RI;
		else
			continue;

		hw_if->enable_int(channel, int_id);
	}
}

static void xgbe_disable_rx_tx_ints(struct xgbe_prv_data *pdata)
{
	struct xgbe_hw_if *hw_if = &pdata->hw_if;
	struct xgbe_channel *channel;
	enum xgbe_int int_id;
	unsigned int i;

	channel = pdata->channel;
	for (i = 0; i < pdata->channel_count; i++, channel++) {
		if (channel->tx_ring && channel->rx_ring)
			int_id = XGMAC_INT_DMA_CH_SR_TI_RI;
		else if (channel->tx_ring)
			int_id = XGMAC_INT_DMA_CH_SR_TI;
		else if (channel->rx_ring)
			int_id = XGMAC_INT_DMA_CH_SR_RI;
		else
			continue;

		hw_if->disable_int(channel, int_id);
	}
}

static irqreturn_t xgbe_isr(int irq, void *data)
{
	struct xgbe_prv_data *pdata = data;
	struct xgbe_hw_if *hw_if = &pdata->hw_if;
	struct xgbe_channel *channel;
	unsigned int dma_isr, dma_ch_isr;
	unsigned int mac_isr, mac_tssr;
	unsigned int i;

	/* The DMA interrupt status register also reports MAC and MTL
	 * interrupts. So for polling mode, we just need to check for
	 * this register to be non-zero
	 */
	dma_isr = XGMAC_IOREAD(pdata, DMA_ISR);
	if (!dma_isr)
		goto isr_done;

	//pr_info("DMA_ISR=%x\n", dma_isr);

	for (i = 0; i < pdata->channel_count; i++) {
		if (!(dma_isr & (1 << i)))
			continue;

		channel = pdata->channel + i;
	
		pr_info("channel=%u\n", i);

		dma_ch_isr = XGMAC_DMA_IOREAD(channel, DMA_CH_SR);
		DBGPR("  DMA_CH%u_ISR = %08x\n", i, dma_ch_isr);

		/* If we get a TI or RI interrupt that means per channel DMA
		 * interrupts are not enabled, so we use the private data napi
		 * structure, not the per channel napi structure
		 */
		if (XGMAC_GET_BITS(dma_ch_isr, DMA_CH_SR, TI) ||
		    XGMAC_GET_BITS(dma_ch_isr, DMA_CH_SR, RI)) {
			if (napi_schedule_prep(&pdata->napi)) {
				/* Disable Tx and Rx interrupts */
				xgbe_disable_rx_tx_ints(pdata);

				/* Turn on polling */
				__napi_schedule(&pdata->napi);
			}
		}

		/* Restart the device on a Fatal Bus Error */
		if (XGMAC_GET_BITS(dma_ch_isr, DMA_CH_SR, FBE))
			schedule_work(&pdata->restart_work);


		/* Clear all interrupt signals */
		XGMAC_DMA_IOWRITE(channel, DMA_CH_SR, dma_ch_isr);
	}
	
	//pr_info("DMA_ISR_MACIS=%x\n", XGMAC_GET_BITS(dma_isr, DMA_ISR, MACIS));
	
	if (XGMAC_GET_BITS(dma_isr, DMA_ISR, MACIS)) {
		mac_isr = XGMAC_IOREAD(pdata, MAC_ISR);
		//pr_info("  MAC_ISR = %08x\n", mac_isr);

		if (XGMAC_GET_BITS(mac_isr, MAC_ISR, TSIS)) {
			mac_tssr = XGMAC_IOREAD(pdata, MAC_TSSR);
			//pr_info("  MAC_TSSR = %08x\n", mac_tssr);

			if (XGMAC_GET_BITS(mac_tssr, MAC_TSSR, TXTSC)) {
				/* Read Tx Timestamp to clear interrupt */
				pdata->tx_tstamp =
					hw_if->get_tx_tstamp(pdata);
				schedule_work(&pdata->tx_tstamp_work);
			}
		}
	}

	//pr_info(" DMA_ISR = %08x\n", XGMAC_IOREAD(pdata, DMA_ISR));

isr_done:
	return IRQ_HANDLED;
}

static irqreturn_t xgbe_dma_isr(int irq, void *data)
{
	struct xgbe_channel *channel = data;

	/* Per channel DMA interrupts are enabled, so we use the per
	 * channel napi structure and not the private data napi structure
	 */
	if (napi_schedule_prep(&channel->napi)) {
		/* Disable Tx and Rx interrupts */
		disable_irq_nosync(channel->dma_irq);

		/* Turn on polling */
		__napi_schedule(&channel->napi);
	}

	return IRQ_HANDLED;
}

static enum hrtimer_restart xgbe_tx_timer(struct hrtimer *timer)
{
	struct xgbe_channel *channel = container_of(timer,
						    struct xgbe_channel,
						    tx_timer);
	struct xgbe_ring *ring = channel->tx_ring;
	struct xgbe_prv_data *pdata = channel->pdata;
	struct napi_struct *napi;
	unsigned long flags;

	DBGPR("-->xgbe_tx_timer\n");

	napi = (pdata->per_channel_irq) ? &channel->napi : &pdata->napi;

	spin_lock_irqsave(&ring->lock, flags);

	if (napi_schedule_prep(napi)) {
		/* Disable Tx and Rx interrupts */
		if (pdata->per_channel_irq)
			disable_irq(channel->dma_irq);
		else
			xgbe_disable_rx_tx_ints(pdata);

		/* Turn on polling */
		__napi_schedule(napi);
	}

	channel->tx_timer_active = 0;

	spin_unlock_irqrestore(&ring->lock, flags);

	DBGPR("<--xgbe_tx_timer\n");

	return HRTIMER_NORESTART;
}

static void xgbe_init_tx_timers(struct xgbe_prv_data *pdata)
{
	struct xgbe_channel *channel;
	unsigned int i;

	DBGPR("-->xgbe_init_tx_timers\n");

	channel = pdata->channel;
	for (i = 0; i < pdata->channel_count; i++, channel++) {
		if (!channel->tx_ring)
			break;

		DBGPR("  %s adding tx timer\n", channel->name);
		hrtimer_init(&channel->tx_timer, CLOCK_MONOTONIC,
			     HRTIMER_MODE_REL);
		channel->tx_timer.function = xgbe_tx_timer;
	}

	DBGPR("<--xgbe_init_tx_timers\n");
}

static void xgbe_stop_tx_timers(struct xgbe_prv_data *pdata)
{
	struct xgbe_channel *channel;
	unsigned int i;

	DBGPR("-->xgbe_stop_tx_timers\n");

	channel = pdata->channel;
	for (i = 0; i < pdata->channel_count; i++, channel++) {
		if (!channel->tx_ring)
			break;

		DBGPR("  %s deleting tx timer\n", channel->name);
		channel->tx_timer_active = 0;
		hrtimer_cancel(&channel->tx_timer);
	}

	DBGPR("<--xgbe_stop_tx_timers\n");
}

void xgbe_get_all_hw_features(struct xgbe_prv_data *pdata)
{
	unsigned int mac_hfr0, mac_hfr1, mac_hfr2;
	struct xgbe_hw_features *hw_feat = &pdata->hw_feat;

	DBGPR("-->xgbe_get_all_hw_features\n");

	mac_hfr0 = XGMAC_IOREAD(pdata, MAC_HWF0R);
	mac_hfr1 = XGMAC_IOREAD(pdata, MAC_HWF1R);
	mac_hfr2 = XGMAC_IOREAD(pdata, MAC_HWF2R);

	memset(hw_feat, 0, sizeof(*hw_feat));

	hw_feat->version = XGMAC_IOREAD(pdata, MAC_VR);

	/* Hardware feature register 0 */
	hw_feat->gmii        = XGMAC_GET_BITS(mac_hfr0, MAC_HWF0R, GMIISEL);
	hw_feat->vlhash      = XGMAC_GET_BITS(mac_hfr0, MAC_HWF0R, VLHASH);
	hw_feat->sma         = XGMAC_GET_BITS(mac_hfr0, MAC_HWF0R, SMASEL);
	hw_feat->rwk         = XGMAC_GET_BITS(mac_hfr0, MAC_HWF0R, RWKSEL);
	hw_feat->mgk         = XGMAC_GET_BITS(mac_hfr0, MAC_HWF0R, MGKSEL);
	hw_feat->aoe         = XGMAC_GET_BITS(mac_hfr0, MAC_HWF0R, ARPOFFSEL);
	hw_feat->ts          = XGMAC_GET_BITS(mac_hfr0, MAC_HWF0R, TSSEL);
	hw_feat->eee         = XGMAC_GET_BITS(mac_hfr0, MAC_HWF0R, EEESEL);
	hw_feat->tx_coe      = XGMAC_GET_BITS(mac_hfr0, MAC_HWF0R, TXCOESEL);
	hw_feat->rx_coe      = XGMAC_GET_BITS(mac_hfr0, MAC_HWF0R, RXCOESEL);
	hw_feat->addn_mac    = XGMAC_GET_BITS(mac_hfr0, MAC_HWF0R,
					      ADDMACADRSEL);
	hw_feat->ts_src      = XGMAC_GET_BITS(mac_hfr0, MAC_HWF0R, TSSTSSEL);
	hw_feat->sa_vlan_ins = XGMAC_GET_BITS(mac_hfr0, MAC_HWF0R, SAVLANINS);

	/* Hardware feature register 1 */
	hw_feat->rx_fifo_size  = XGMAC_GET_BITS(mac_hfr1, MAC_HWF1R,
						RXFIFOSIZE);
	hw_feat->tx_fifo_size  = XGMAC_GET_BITS(mac_hfr1, MAC_HWF1R,
						TXFIFOSIZE);
	hw_feat->dcb           = XGMAC_GET_BITS(mac_hfr1, MAC_HWF1R, DCBEN);
	hw_feat->sph           = XGMAC_GET_BITS(mac_hfr1, MAC_HWF1R, SPHEN);
	hw_feat->tso           = XGMAC_GET_BITS(mac_hfr1, MAC_HWF1R, TSOEN);
	hw_feat->dma_debug     = XGMAC_GET_BITS(mac_hfr1, MAC_HWF1R, DBGMEMA);
	hw_feat->rss           = XGMAC_GET_BITS(mac_hfr1, MAC_HWF1R, RSSEN);
	hw_feat->tc_cnt	       = XGMAC_GET_BITS(mac_hfr1, MAC_HWF1R, NUMTC);
	hw_feat->hash_table_size = XGMAC_GET_BITS(mac_hfr1, MAC_HWF1R,
						  HASHTBLSZ);
	hw_feat->l3l4_filter_num = XGMAC_GET_BITS(mac_hfr1, MAC_HWF1R,
						  L3L4FNUM);

	/* Hardware feature register 2 */
	hw_feat->rx_q_cnt     = XGMAC_GET_BITS(mac_hfr2, MAC_HWF2R, RXQCNT);
	hw_feat->tx_q_cnt     = XGMAC_GET_BITS(mac_hfr2, MAC_HWF2R, TXQCNT);
	hw_feat->rx_ch_cnt    = XGMAC_GET_BITS(mac_hfr2, MAC_HWF2R, RXCHCNT);
	hw_feat->tx_ch_cnt    = XGMAC_GET_BITS(mac_hfr2, MAC_HWF2R, TXCHCNT);
	hw_feat->pps_out_num  = XGMAC_GET_BITS(mac_hfr2, MAC_HWF2R, PPSOUTNUM);
	hw_feat->aux_snap_num = XGMAC_GET_BITS(mac_hfr2, MAC_HWF2R, AUXSNAPNUM);

	/* Translate the Hash Table size into actual number */
	switch (hw_feat->hash_table_size) {
	case 0:
		break;
	case 1:
		hw_feat->hash_table_size = 64;
		break;
	case 2:
		hw_feat->hash_table_size = 128;
		break;
	case 3:
		hw_feat->hash_table_size = 256;
		break;
	}

	/* The Queue, Channel and TC counts are zero based so increment them
	 * to get the actual number
	 */
	hw_feat->rx_q_cnt++;
	hw_feat->tx_q_cnt++;
	hw_feat->rx_ch_cnt++;
	hw_feat->tx_ch_cnt++;
	hw_feat->tc_cnt++;

	DBGPR("<--xgbe_get_all_hw_features\n");
}

static void xgbe_napi_enable(struct xgbe_prv_data *pdata, unsigned int add)
{
	struct xgbe_channel *channel;
	unsigned int i;

	if (pdata->per_channel_irq) {
		channel = pdata->channel;
		for (i = 0; i < pdata->channel_count; i++, channel++) {
			if (add)
				netif_napi_add(pdata->netdev, &channel->napi,
					       xgbe_one_poll, NAPI_POLL_WEIGHT);

			napi_enable(&channel->napi);
		}
	} else {
		if (add)
			netif_napi_add(pdata->netdev, &pdata->napi,
				       xgbe_all_poll, NAPI_POLL_WEIGHT);

		napi_enable(&pdata->napi);
	}
}

static void xgbe_napi_disable(struct xgbe_prv_data *pdata, unsigned int del)
{
	struct xgbe_channel *channel;
	unsigned int i;

	if (pdata->per_channel_irq) {
		channel = pdata->channel;
		for (i = 0; i < pdata->channel_count; i++, channel++) {
			napi_disable(&channel->napi);

			if (del)
				netif_napi_del(&channel->napi);
		}
	} else {
		napi_disable(&pdata->napi);

		if (del)
			netif_napi_del(&pdata->napi);
	}
}

void xgbe_init_tx_coalesce(struct xgbe_prv_data *pdata)
{
	struct xgbe_hw_if *hw_if = &pdata->hw_if;

	DBGPR("-->xgbe_init_tx_coalesce\n");

	pdata->tx_usecs = XGMAC_INIT_DMA_TX_USECS;
	pdata->tx_frames = XGMAC_INIT_DMA_TX_FRAMES;

	hw_if->config_tx_coalesce(pdata);

	DBGPR("<--xgbe_init_tx_coalesce\n");
}

void xgbe_init_rx_coalesce(struct xgbe_prv_data *pdata)
{
	struct xgbe_hw_if *hw_if = &pdata->hw_if;

	DBGPR("-->xgbe_init_rx_coalesce\n");

	pdata->rx_riwt = hw_if->usec_to_riwt(pdata, XGMAC_INIT_DMA_RX_USECS);
	pdata->rx_frames = XGMAC_INIT_DMA_RX_FRAMES;

	hw_if->config_rx_coalesce(pdata);

	DBGPR("<--xgbe_init_rx_coalesce\n");
}

static void xgbe_free_tx_data(struct xgbe_prv_data *pdata)
{
	struct xgbe_desc_if *desc_if = &pdata->desc_if;
	struct xgbe_channel *channel;
	struct xgbe_ring *ring;
	struct xgbe_ring_data *rdata;
	unsigned int i, j;

	DBGPR("-->xgbe_free_tx_data\n");

	channel = pdata->channel;
	for (i = 0; i < pdata->channel_count; i++, channel++) {
		ring = channel->tx_ring;
		if (!ring)
			break;

		for (j = 0; j < ring->rdesc_count; j++) {
			rdata = XGBE_GET_DESC_DATA(ring, j);
			desc_if->unmap_rdata(pdata, rdata);
		}
	}

	DBGPR("<--xgbe_free_tx_data\n");
}

static void xgbe_free_rx_data(struct xgbe_prv_data *pdata)
{
	struct xgbe_desc_if *desc_if = &pdata->desc_if;
	struct xgbe_channel *channel;
	struct xgbe_ring *ring;
	struct xgbe_ring_data *rdata;
	unsigned int i, j;

	DBGPR("-->xgbe_free_rx_data\n");

	channel = pdata->channel;
	for (i = 0; i < pdata->channel_count; i++, channel++) {
		ring = channel->rx_ring;
		if (!ring)
			break;

		for (j = 0; j < ring->rdesc_count; j++) {
			rdata = XGBE_GET_DESC_DATA(ring, j);
			desc_if->unmap_rdata(pdata, rdata);
		}
	}

	DBGPR("<--xgbe_free_rx_data\n");
}

static void xgbe_adjust_link(struct net_device *netdev)
{
	struct xgbe_prv_data *pdata = netdev_priv(netdev);
	struct xgbe_hw_if *hw_if = &pdata->hw_if;
	struct phy_device *phydev = pdata->phydev;
	int new_state = 0;

	if (phydev == NULL)
		return;

	if (phydev->link) {
//		netdev_info(netdev, "phy_link=1\n");
		/* Flow control support */
		if (pdata->pause_autoneg) {
			if (phydev->pause || phydev->asym_pause) {
				pdata->tx_pause = 1;
				pdata->rx_pause = 1;
			} else {
				pdata->tx_pause = 0;
				pdata->rx_pause = 0;
			}
		}

		if (pdata->tx_pause != pdata->phy_tx_pause) {
			netdev_info(netdev, "config_tx_flow_control\n");
			hw_if->config_tx_flow_control(pdata);
			pdata->phy_tx_pause = pdata->tx_pause;
		}

		if (pdata->rx_pause != pdata->phy_rx_pause) {
			netdev_info(netdev, "config_rx_flow_control\n");
			hw_if->config_rx_flow_control(pdata);
			pdata->phy_rx_pause = pdata->rx_pause;
		}

		/* Speed support */
		if (phydev->speed != pdata->phy_speed) {
			new_state = 1;

			switch (phydev->speed) {
			case SPEED_10000:
				netdev_info(netdev, "set_xgmii_speed\n");
				hw_if->set_xgmii_speed(pdata);
				break;

			case SPEED_2500:
				hw_if->set_gmii_2500_speed(pdata);
				break;

			case SPEED_1000:
				hw_if->set_gmii_speed(pdata);
				break;
			}
			pdata->phy_speed = phydev->speed;
		}

		if (phydev->link != pdata->phy_link) {
			new_state = 1;
			pdata->phy_link = 1;
		}
	} else if (pdata->phy_link) {
		new_state = 1;
		pdata->phy_link = 0;
		pdata->phy_speed = SPEED_UNKNOWN;
	}

	if (new_state)
		phy_print_status(phydev);
}

static int xgbe_phy_init(struct xgbe_prv_data *pdata)
{
	struct net_device *netdev = pdata->netdev;
	struct phy_device *phydev = pdata->phydev;
	int ret;

	pdata->phy_link = -1;
	pdata->phy_speed = SPEED_UNKNOWN;
	pdata->phy_tx_pause = pdata->tx_pause;
	pdata->phy_rx_pause = pdata->rx_pause;

	ret = phy_connect_direct(netdev, phydev, &xgbe_adjust_link,
				 pdata->phy_mode);
	if (ret) {
		netdev_err(netdev, "phy_connect_direct failed\n");
		return ret;
	}

	if (!phydev->drv || (phydev->drv->phy_id == 0)) {
		netdev_err(netdev, "phy_id not valid\n");
		ret = -ENODEV;
		goto err_phy_connect;
	}
	DBGPR("  phy_connect_direct succeeded for PHY %s, link=%d\n",
	      dev_name(&phydev->dev), phydev->link);

	return 0;

err_phy_connect:
	phy_disconnect(phydev);

	return ret;
}

static void xgbe_phy_exit(struct xgbe_prv_data *pdata)
{
	if (!pdata->phydev)
		return;

	phy_disconnect(pdata->phydev);
}

int xgbe_powerdown(struct net_device *netdev, unsigned int caller)
{
	struct xgbe_prv_data *pdata = netdev_priv(netdev);
	struct xgbe_hw_if *hw_if = &pdata->hw_if;
	unsigned long flags;

	DBGPR("-->xgbe_powerdown\n");

	if (!netif_running(netdev) ||
	    (caller == XGMAC_IOCTL_CONTEXT && pdata->power_down)) {
		netdev_alert(netdev, "Device is already powered down\n");
		DBGPR("<--xgbe_powerdown\n");
		return -EINVAL;
	}

	phy_stop(pdata->phydev);

	spin_lock_irqsave(&pdata->lock, flags);

	if (caller == XGMAC_DRIVER_CONTEXT)
		netif_device_detach(netdev);

	netif_tx_stop_all_queues(netdev);
	xgbe_napi_disable(pdata, 0);

	/* Powerdown Tx/Rx */
	hw_if->powerdown_tx(pdata);
	hw_if->powerdown_rx(pdata);

	pdata->power_down = 1;

	spin_unlock_irqrestore(&pdata->lock, flags);

	DBGPR("<--xgbe_powerdown\n");

	return 0;
}

int xgbe_powerup(struct net_device *netdev, unsigned int caller)
{
	struct xgbe_prv_data *pdata = netdev_priv(netdev);
	struct xgbe_hw_if *hw_if = &pdata->hw_if;
	unsigned long flags;

	DBGPR("-->xgbe_powerup\n");

	if (!netif_running(netdev) ||
	    (caller == XGMAC_IOCTL_CONTEXT && !pdata->power_down)) {
		netdev_alert(netdev, "Device is already powered up\n");
		DBGPR("<--xgbe_powerup\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&pdata->lock, flags);

	pdata->power_down = 0;

	phy_start(pdata->phydev);

	/* Enable Tx/Rx */
	hw_if->powerup_tx(pdata);
	hw_if->powerup_rx(pdata);

	if (caller == XGMAC_DRIVER_CONTEXT)
		netif_device_attach(netdev);

	xgbe_napi_enable(pdata, 0);
	netif_tx_start_all_queues(netdev);

	spin_unlock_irqrestore(&pdata->lock, flags);

	DBGPR("<--xgbe_powerup\n");

	return 0;
}

static int xgbe_start(struct xgbe_prv_data *pdata)
{
	struct xgbe_hw_if *hw_if = &pdata->hw_if;
	struct net_device *netdev = pdata->netdev;

	DBGPR("-->xgbe_start\n");

	xgbe_set_rx_mode(netdev);

	hw_if->init(pdata);

	phy_start(pdata->phydev);

	hw_if->enable_tx(pdata);
	hw_if->enable_rx(pdata);

	xgbe_init_tx_timers(pdata);

	xgbe_napi_enable(pdata, 1);
	netif_tx_start_all_queues(netdev);

	DBGPR("<--xgbe_start\n");

	return 0;
}

static void xgbe_stop(struct xgbe_prv_data *pdata)
{
	struct xgbe_hw_if *hw_if = &pdata->hw_if;
	struct xgbe_channel *channel;
	struct net_device *netdev = pdata->netdev;
	struct netdev_queue *txq;
	unsigned int i;

	DBGPR("-->xgbe_stop\n");

	phy_stop(pdata->phydev);

	netif_tx_stop_all_queues(netdev);
	xgbe_napi_disable(pdata, 1);

	xgbe_stop_tx_timers(pdata);

	hw_if->disable_tx(pdata);
	hw_if->disable_rx(pdata);

	channel = pdata->channel;
	for (i = 0; i < pdata->channel_count; i++, channel++) {
		if (!channel->tx_ring)
			continue;

		txq = netdev_get_tx_queue(netdev, channel->queue_index);
		netdev_tx_reset_queue(txq);
	}

	DBGPR("<--xgbe_stop\n");
}

static void xgbe_restart_dev(struct xgbe_prv_data *pdata, unsigned int reset)
{
	struct xgbe_channel *channel;
	struct xgbe_hw_if *hw_if = &pdata->hw_if;
	unsigned int i;

	DBGPR("-->xgbe_restart_dev\n");

	/* If not running, "restart" will happen on open */
	if (!netif_running(pdata->netdev))
		return;

	xgbe_stop(pdata);
	synchronize_irq(pdata->dev_irq);
	if (pdata->per_channel_irq) {
		channel = pdata->channel;
		for (i = 0; i < pdata->channel_count; i++, channel++)
			synchronize_irq(channel->dma_irq);
	}

	xgbe_free_tx_data(pdata);
	xgbe_free_rx_data(pdata);

	/* Issue software reset to device if requested */
	if (reset)
		hw_if->exit(pdata);

	xgbe_start(pdata);

	DBGPR("<--xgbe_restart_dev\n");
}

static void xgbe_restart(struct work_struct *work)
{
	struct xgbe_prv_data *pdata = container_of(work,
						   struct xgbe_prv_data,
						   restart_work);

	rtnl_lock();

	xgbe_restart_dev(pdata, 1);

	rtnl_unlock();
}

static void xgbe_tx_tstamp(struct work_struct *work)
{
	struct xgbe_prv_data *pdata = container_of(work,
						   struct xgbe_prv_data,
						   tx_tstamp_work);
	struct skb_shared_hwtstamps hwtstamps;
	u64 nsec;
	unsigned long flags;

	if (pdata->tx_tstamp) {
		nsec = timecounter_cyc2time(&pdata->tstamp_tc,
					    pdata->tx_tstamp);

		memset(&hwtstamps, 0, sizeof(hwtstamps));
		hwtstamps.hwtstamp = ns_to_ktime(nsec);
		skb_tstamp_tx(pdata->tx_tstamp_skb, &hwtstamps);
	}

	dev_kfree_skb_any(pdata->tx_tstamp_skb);

	spin_lock_irqsave(&pdata->tstamp_lock, flags);
	pdata->tx_tstamp_skb = NULL;
	spin_unlock_irqrestore(&pdata->tstamp_lock, flags);
}

static int xgbe_get_hwtstamp_settings(struct xgbe_prv_data *pdata,
				      struct ifreq *ifreq)
{
	if (copy_to_user(ifreq->ifr_data, &pdata->tstamp_config,
			 sizeof(pdata->tstamp_config)))
		return -EFAULT;

	return 0;
}

static int xgbe_set_hwtstamp_settings(struct xgbe_prv_data *pdata,
				      struct ifreq *ifreq)
{
	struct hwtstamp_config config;
	unsigned int mac_tscr;

	if (copy_from_user(&config, ifreq->ifr_data, sizeof(config)))
		return -EFAULT;

	if (config.flags)
		return -EINVAL;

	mac_tscr = 0;

	switch (config.tx_type) {
	case HWTSTAMP_TX_OFF:
		break;

	case HWTSTAMP_TX_ON:
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSENA, 1);
		break;

	default:
		return -ERANGE;
	}

	switch (config.rx_filter) {
	case HWTSTAMP_FILTER_NONE:
		break;

	case HWTSTAMP_FILTER_ALL:
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSENALL, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSENA, 1);
		break;

	/* PTP v2, UDP, any kind of event packet */
	case HWTSTAMP_FILTER_PTP_V2_L4_EVENT:
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSVER2ENA, 1);
	/* PTP v1, UDP, any kind of event packet */
	case HWTSTAMP_FILTER_PTP_V1_L4_EVENT:
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSIPV4ENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSIPV6ENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, SNAPTYPSEL, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSENA, 1);
		break;

	/* PTP v2, UDP, Sync packet */
	case HWTSTAMP_FILTER_PTP_V2_L4_SYNC:
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSVER2ENA, 1);
	/* PTP v1, UDP, Sync packet */
	case HWTSTAMP_FILTER_PTP_V1_L4_SYNC:
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSIPV4ENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSIPV6ENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSEVNTENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSENA, 1);
		break;

	/* PTP v2, UDP, Delay_req packet */
	case HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ:
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSVER2ENA, 1);
	/* PTP v1, UDP, Delay_req packet */
	case HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ:
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSIPV4ENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSIPV6ENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSEVNTENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSMSTRENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSENA, 1);
		break;

	/* 802.AS1, Ethernet, any kind of event packet */
	case HWTSTAMP_FILTER_PTP_V2_L2_EVENT:
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, AV8021ASMEN, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, SNAPTYPSEL, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSENA, 1);
		break;

	/* 802.AS1, Ethernet, Sync packet */
	case HWTSTAMP_FILTER_PTP_V2_L2_SYNC:
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, AV8021ASMEN, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSEVNTENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSENA, 1);
		break;

	/* 802.AS1, Ethernet, Delay_req packet */
	case HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ:
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, AV8021ASMEN, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSMSTRENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSEVNTENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSENA, 1);
		break;

	/* PTP v2/802.AS1, any layer, any kind of event packet */
	case HWTSTAMP_FILTER_PTP_V2_EVENT:
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSVER2ENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSIPENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSIPV4ENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSIPV6ENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, SNAPTYPSEL, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSENA, 1);
		break;

	/* PTP v2/802.AS1, any layer, Sync packet */
	case HWTSTAMP_FILTER_PTP_V2_SYNC:
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSVER2ENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSIPENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSIPV4ENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSIPV6ENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSEVNTENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSENA, 1);
		break;

	/* PTP v2/802.AS1, any layer, Delay_req packet */
	case HWTSTAMP_FILTER_PTP_V2_DELAY_REQ:
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSVER2ENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSIPENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSIPV4ENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSIPV6ENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSMSTRENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSEVNTENA, 1);
		XGMAC_SET_BITS(mac_tscr, MAC_TSCR, TSENA, 1);
		break;

	default:
		return -ERANGE;
	}

	pdata->hw_if.config_tstamp(pdata, mac_tscr);

	memcpy(&pdata->tstamp_config, &config, sizeof(config));

	return 0;
}

static void xgbe_prep_tx_tstamp(struct xgbe_prv_data *pdata,
				struct sk_buff *skb,
				struct xgbe_packet_data *packet)
{
	unsigned long flags;

	if (XGMAC_GET_BITS(packet->attributes, TX_PACKET_ATTRIBUTES, PTP)) {
		spin_lock_irqsave(&pdata->tstamp_lock, flags);
		if (pdata->tx_tstamp_skb) {
			/* Another timestamp in progress, ignore this one */
			XGMAC_SET_BITS(packet->attributes,
				       TX_PACKET_ATTRIBUTES, PTP, 0);
		} else {
			pdata->tx_tstamp_skb = skb_get(skb);
			skb_shinfo(skb)->tx_flags |= SKBTX_IN_PROGRESS;
		}
		spin_unlock_irqrestore(&pdata->tstamp_lock, flags);
	}

	if (!XGMAC_GET_BITS(packet->attributes, TX_PACKET_ATTRIBUTES, PTP))
		skb_tx_timestamp(skb);
}

static void xgbe_prep_vlan(struct sk_buff *skb, struct xgbe_packet_data *packet)
{
	if (vlan_tx_tag_present(skb))
		packet->vlan_ctag = vlan_tx_tag_get(skb);
}

static int xgbe_prep_tso(struct sk_buff *skb, struct xgbe_packet_data *packet)
{
	int ret;

	if (!XGMAC_GET_BITS(packet->attributes, TX_PACKET_ATTRIBUTES,
			    TSO_ENABLE))
		return 0;

	ret = skb_cow_head(skb, 0);
	if (ret)
		return ret;

	packet->header_len = skb_transport_offset(skb) + tcp_hdrlen(skb);
	packet->tcp_header_len = tcp_hdrlen(skb);
	packet->tcp_payload_len = skb->len - packet->header_len;
	packet->mss = skb_shinfo(skb)->gso_size;
	DBGPR("  packet->header_len=%u\n", packet->header_len);
	DBGPR("  packet->tcp_header_len=%u, packet->tcp_payload_len=%u\n",
	      packet->tcp_header_len, packet->tcp_payload_len);
	DBGPR("  packet->mss=%u\n", packet->mss);

	/* Update the number of packets that will ultimately be transmitted
	 * along with the extra bytes for each extra packet
	 */
	packet->tx_packets = skb_shinfo(skb)->gso_segs;
	packet->tx_bytes += (packet->tx_packets - 1) * packet->header_len;

	return 0;
}

static int xgbe_is_tso(struct sk_buff *skb)
{
	if (skb->ip_summed != CHECKSUM_PARTIAL)
		return 0;

	if (!skb_is_gso(skb))
		return 0;

	DBGPR("  TSO packet to be processed\n");

	return 1;
}

static void xgbe_packet_info(struct xgbe_prv_data *pdata,
			     struct xgbe_ring *ring, struct sk_buff *skb,
			     struct xgbe_packet_data *packet)
{
	struct skb_frag_struct *frag;
	unsigned int context_desc;
	unsigned int len;
	unsigned int i;

	packet->skb = skb;

	context_desc = 0;
	packet->rdesc_count = 0;

	packet->tx_packets = 1;
	packet->tx_bytes = skb->len;

	if (xgbe_is_tso(skb)) {
		/* TSO requires an extra descriptor if mss is different */
		if (skb_shinfo(skb)->gso_size != ring->tx.cur_mss) {
			context_desc = 1;
			packet->rdesc_count++;
		}

		/* TSO requires an extra descriptor for TSO header */
		packet->rdesc_count++;

		XGMAC_SET_BITS(packet->attributes, TX_PACKET_ATTRIBUTES,
			       TSO_ENABLE, 1);
		XGMAC_SET_BITS(packet->attributes, TX_PACKET_ATTRIBUTES,
			       CSUM_ENABLE, 1);
	} else if (skb->ip_summed == CHECKSUM_PARTIAL)
		XGMAC_SET_BITS(packet->attributes, TX_PACKET_ATTRIBUTES,
			       CSUM_ENABLE, 1);

	if (vlan_tx_tag_present(skb)) {
		/* VLAN requires an extra descriptor if tag is different */
		if (vlan_tx_tag_get(skb) != ring->tx.cur_vlan_ctag)
			/* We can share with the TSO context descriptor */
			if (!context_desc) {
				context_desc = 1;
				packet->rdesc_count++;
			}

		XGMAC_SET_BITS(packet->attributes, TX_PACKET_ATTRIBUTES,
			       VLAN_CTAG, 1);
	}

	if ((skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP) &&
	    (pdata->tstamp_config.tx_type == HWTSTAMP_TX_ON))
		XGMAC_SET_BITS(packet->attributes, TX_PACKET_ATTRIBUTES,
			       PTP, 1);

	for (len = skb_headlen(skb); len;) {
		packet->rdesc_count++;
		len -= min_t(unsigned int, len, XGBE_TX_MAX_BUF_SIZE);
	}

	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
		frag = &skb_shinfo(skb)->frags[i];
		for (len = skb_frag_size(frag); len; ) {
			packet->rdesc_count++;
			len -= min_t(unsigned int, len, XGBE_TX_MAX_BUF_SIZE);
		}
	}
}

static int xgbe_open(struct net_device *netdev)
{
	struct xgbe_prv_data *pdata = netdev_priv(netdev);
	struct xgbe_hw_if *hw_if = &pdata->hw_if;
	struct xgbe_desc_if *desc_if = &pdata->desc_if;
	struct xgbe_channel *channel = NULL;
	unsigned int i = 0;
	int ret;
	uint32_t reg;
	int poll;
	int st = 0;

DBGPR("-->%s\n", __func__);





	/* Enable the clocks */
	ret = clk_prepare_enable(pdata->sysclk);
	if (ret) {
		netdev_alert(netdev, "dma clk_prepare_enable failed\n");
		goto err_phy_init;
	}

	ret = clk_prepare_enable(pdata->ptpclk);
	if (ret) {
		netdev_alert(netdev, "ptp clk_prepare_enable failed\n");
		goto err_sysclk;
	}

	/* Initialize the phy */
	ret = xgbe_phy_init(pdata);
	if (ret)
		return ret;


	DBGPR("XPCS Initializing... \n\r");

	/* 4) Read the SR XS or PCS MMD Control1 Register and wait till the 15 th
	bit of this register (RST – Soft Reset) is read as 0. */
	i = 0;

	do {
		poll = READ_XPCS_REG(DEVICE_PCS, SR_XS_or_PCS_MMD_Control1) & SR_XS_or_PCS_MMD_Control1_RST;
		DBGPR("RST wait poll = %d\n\r", poll);
		if (i++ > MAX_POLLING_TIMES)
			break;
	} while (poll);

	DBGPR("Clear Soft Reset[15] = %d.\n\r", poll);

	/*5) Set 12 th bit (AN_EN) to 0 in SR AN MMD Control Register to disable
	Clause 73 auto-negotiation. */
//	reg = READ_XPCS_REG(DEVICE_AN, SR_AN_MMD_Control);
//	reg &= ~SR_AN_MMD_Control_AN_EN;
//	WRITE_XPCS_REG(DEVICE_AN, SR_AN_MMD_Control, reg);

	/* 6) Program bits [1:0] (PCS_TYPE_SEL) in SR PCS Control2 Register to
	2’b01 to select KX4 mode or to 2’b00 to select KR mode of operation. */
	reg = READ_XPCS_REG(DEVICE_PCS, SR_PCS_Control2);
	reg &= ~SR_PCS_Control2_PCS_TYPE_SEL_MASK;
	reg |= SR_PCS_Control2_PCS_TYPE_SEL_KR;
	WRITE_XPCS_REG(DEVICE_PCS, SR_PCS_Control2, reg);

	/* If KX4 mode of operation is in use, the following steps have to be done:
	1) Program field LANE_MODE (bits [3:0]) to 4 and field LINK_WIDTH (bits [10:8])
	 to 2 in VR XS,PMA or MII Synopsys Enterprise Gen5 10G PHY General Control Register. */
	reg = READ_XPCS_REG(DEVICE_PMA, VR_XS_PMA_MII_Ent_Gen5_Gen_Ctl);
	reg &= ~VR_XS_PMA_MII_Ent_Gen5_Gen_Ctl_LANE_MODE_MASK & \
	~(VR_XS_PMA_MII_Ent_Gen5_Gen_Ctl_LINK_WIDTH_MASK << VR_XS_PMA_MII_Ent_Gen5_Gen_Ctl_LINK_WIDTH_SHIFT);
	reg |= VR_XS_PMA_MII_Ent_Gen5_Gen_Ctl_LANE_MODE_KX4 | \
	(VR_XS_PMA_MII_Ent_Gen5_Gen_Ctl_LINK_WIDTH_4 << VR_XS_PMA_MII_Ent_Gen5_Gen_Ctl_LINK_WIDTH_SHIFT);
//	WRITE_XPCS_REG(DEVICE_PMA, VR_XS_PMA_MII_Ent_Gen5_Gen_Ctl, reg);

	/* Enable loopback mode. */
	reg = READ_XPCS_REG(DEVICE_PMA, SR_PMA_MMD_Control1);
	reg |= SR_PMA_MMD_Control1_LB;
	DBGPR("SR_PMA_MMD_Control1 = 0x%x.\n\r", reg);
//	WRITE_XPCS_REG(DEVICE_PMA, SR_PMA_MMD_Control1, reg);

	/* 2) Initiate the Vendor specific Software Reset by setting VR_RST bit[15]
	 of VR XS or PCS MMD Digital Control1 Register to 0x1.*/
	reg = READ_XPCS_REG(DEVICE_PCS, VR_XS_or_PCS_MMD_Digital_Ctl1);
	reg |= VR_XS_or_PCS_MMD_Digital_Ctl1_VR_RST;
	WRITE_XPCS_REG(DEVICE_PCS, VR_XS_or_PCS_MMD_Digital_Ctl1, reg);

	/* 3) Wait for the bit[15] (VR_RST) of the VR XS or PCS MMD Digital Control1
	Register to get cleared.*/

//	read_xpcs_regs_drv();
	i = 0;
	do {
		poll = READ_XPCS_REG(DEVICE_PCS, VR_XS_or_PCS_MMD_Digital_Ctl1) & VR_XS_or_PCS_MMD_Digital_Ctl1_VR_RST;
		if (i++ > MAX_POLLING_TIMES)
			break;
	} while (poll);

	DBGPR("Clear Bit VR_RST=%d.\n\r", poll);

	/* In the initialization process host should program all OUI, MMN and RN
	 fields in Control MMD Standard Registers. Appropriate values of this fields
	  are in BE-T-B_T-AS-008-ID document. */

	reg = SR_Ctl_MMD_REG_1_VAL;
	WRITE_XPCS_REG(DEVICE_Vendor_Specific, SR_Ctl_MMD_PMA_Dev_Id_1, reg);
	WRITE_XPCS_REG(DEVICE_Vendor_Specific, SR_Ctl_MMD_Dev_Id_1, reg);
	WRITE_XPCS_REG(DEVICE_Vendor_Specific, SR_Ctl_MMD_PCS_Dev_Id_1, reg);
	WRITE_XPCS_REG(DEVICE_Vendor_Specific, SR_Ctl_MMD_AN_Dev_Id_1, reg);
	WRITE_XPCS_REG(DEVICE_Vendor_Specific, SR_Ctl_MMD_Package_Id_1, reg);

	st += (READ_XPCS_REG(DEVICE_Vendor_Specific, SR_Ctl_MMD_PMA_Dev_Id_1) != reg);
	st += (READ_XPCS_REG(DEVICE_Vendor_Specific, SR_Ctl_MMD_Dev_Id_1) != reg);
	st += (READ_XPCS_REG(DEVICE_Vendor_Specific, SR_Ctl_MMD_PCS_Dev_Id_1) != reg);
	st += (READ_XPCS_REG(DEVICE_Vendor_Specific, SR_Ctl_MMD_AN_Dev_Id_1) != reg);
	st += (READ_XPCS_REG(DEVICE_Vendor_Specific, SR_Ctl_MMD_Package_Id_1) != reg);

	WRITE_XPCS_REG(DEVICE_Vendor_Specific, SR_Ctl_MMD_PMA_Dev_Id_2, SR_Ctl_MMD_PMA_Dev_Id_2_VAL);
	WRITE_XPCS_REG(DEVICE_Vendor_Specific, SR_Ctl_MMD_Dev_Id_2, SR_Ctl_MMD_Dev_Id_2_VAL );
	WRITE_XPCS_REG(DEVICE_Vendor_Specific, SR_Ctl_MMD_PCS_Dev_Id_2,SR_Ctl_MMD_PCS_Dev_Id_2_VAL );
	WRITE_XPCS_REG(DEVICE_Vendor_Specific, SR_Ctl_MMD_AN_Dev_Id_2, SR_Ctl_MMD_AN_Dev_Id_2_VAL );
	WRITE_XPCS_REG(DEVICE_Vendor_Specific, SR_Ctl_MMD_Package_Id_2, SR_Ctl_MMD_Package_Id_2_VAL);

	st += (READ_XPCS_REG(DEVICE_Vendor_Specific, SR_Ctl_MMD_PMA_Dev_Id_2) != SR_Ctl_MMD_PMA_Dev_Id_2_VAL);
	st += (READ_XPCS_REG(DEVICE_Vendor_Specific, SR_Ctl_MMD_Dev_Id_2) != SR_Ctl_MMD_Dev_Id_2_VAL );
	st += (READ_XPCS_REG(DEVICE_Vendor_Specific, SR_Ctl_MMD_PCS_Dev_Id_2) != SR_Ctl_MMD_PCS_Dev_Id_2_VAL);
	st += (READ_XPCS_REG(DEVICE_Vendor_Specific, SR_Ctl_MMD_AN_Dev_Id_2) != SR_Ctl_MMD_AN_Dev_Id_2_VAL);
	st += (READ_XPCS_REG(DEVICE_Vendor_Specific, SR_Ctl_MMD_Package_Id_2) != SR_Ctl_MMD_Package_Id_2_VAL);

	/* 4) Wait for RLU bit[2] of SR XS or PCS MMD Status1 Register to be set to 1. */
	i = 0;
	do {
		poll = READ_XPCS_REG(DEVICE_PCS, SR_XS_or_PCS_MMD_Status1) & SR_XS_or_PCS_MMD_Status1_RLU;
		if (i++ > MAX_POLLING_TIMES)
			break;
	} while (!poll);


	DBGPR("\n\nAND AFTER ALL \n\n");

//	read_xpcs_regs_drv();


	DBGPR("Set Bit RLU = %d.\n\r", poll);
	/* Calculate the Rx buffer size before allocating rings */
	ret = xgbe_calc_rx_buf_size(netdev, netdev->mtu);
	if (ret < 0)
		goto err_ptpclk;
	pdata->rx_buf_size = ret;

	/* Allocate the channel and ring structures */
	ret = xgbe_alloc_channels(pdata);
	if (ret)
		goto err_ptpclk;

	/* Allocate the ring descriptors and buffers */
	ret = desc_if->alloc_ring_resources(pdata);
	if (ret)
		goto err_channels;

	/* Initialize the device restart and Tx timestamp work struct */
	INIT_WORK(&pdata->restart_work, xgbe_restart);
	INIT_WORK(&pdata->tx_tstamp_work, xgbe_tx_tstamp);

	/* Request interrupts */
	ret = devm_request_irq(pdata->dev, pdata->dev_irq, xgbe_isr, 0,
			       netdev->name, pdata);
	if (ret) {
		netdev_alert(netdev, "error requesting irq %d\n",
			     pdata->dev_irq);
		goto err_rings;
	}

	if (pdata->per_channel_irq) {
		channel = pdata->channel;
		for (i = 0; i < pdata->channel_count; i++, channel++) {
			snprintf(channel->dma_irq_name,
				 sizeof(channel->dma_irq_name) - 1,
				 "%s-TxRx-%u", netdev_name(netdev),
				 channel->queue_index);

			ret = devm_request_irq(pdata->dev, channel->dma_irq,
					       xgbe_dma_isr, 0,
					       channel->dma_irq_name, channel);
			if (ret) {
				netdev_alert(netdev,
					     "error requesting irq %d\n",
					     channel->dma_irq);
				goto err_irq;
			}
		}
	}

	ret = xgbe_start(pdata);
	if (ret)
		goto err_start;

	DBGPR("<--xgbe_open\n");

	return 0;

err_start:
	hw_if->exit(pdata);

err_irq:
	if (pdata->per_channel_irq) {
		/* Using an unsigned int, 'i' will go to UINT_MAX and exit */
		for (i--, channel--; i < pdata->channel_count; i--, channel--)
			devm_free_irq(pdata->dev, channel->dma_irq, channel);
	}

	devm_free_irq(pdata->dev, pdata->dev_irq, pdata);

err_rings:
	desc_if->free_ring_resources(pdata);

err_channels:
	xgbe_free_channels(pdata);

err_ptpclk:
	clk_disable_unprepare(pdata->ptpclk);

err_sysclk:
	clk_disable_unprepare(pdata->sysclk);

err_phy_init:
	xgbe_phy_exit(pdata);

	return ret;
}

static int xgbe_close(struct net_device *netdev)
{
	struct xgbe_prv_data *pdata = netdev_priv(netdev);
	struct xgbe_hw_if *hw_if = &pdata->hw_if;
	struct xgbe_desc_if *desc_if = &pdata->desc_if;
	struct xgbe_channel *channel;
	unsigned int i;

	DBGPR("-->xgbe_close\n");

	/* Stop the device */
	xgbe_stop(pdata);

	/* Issue software reset to device */
	hw_if->exit(pdata);

	/* Free the ring descriptors and buffers */
	desc_if->free_ring_resources(pdata);

	/* Release the interrupts */
	devm_free_irq(pdata->dev, pdata->dev_irq, pdata);
	if (pdata->per_channel_irq) {
		channel = pdata->channel;
		for (i = 0; i < pdata->channel_count; i++, channel++)
			devm_free_irq(pdata->dev, channel->dma_irq, channel);
	}

	/* Free the channel and ring structures */
	xgbe_free_channels(pdata);

	/* Disable the clocks */
	clk_disable_unprepare(pdata->ptpclk);
	clk_disable_unprepare(pdata->sysclk);

	/* Release the phy */
	xgbe_phy_exit(pdata);

	DBGPR("<--xgbe_close\n");

	return 0;
}

static int xgbe_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	struct xgbe_prv_data *pdata = netdev_priv(netdev);
	struct xgbe_stats *pstats = &pdata->stats;
	struct xgbe_hw_if *hw_if = &pdata->hw_if;
	struct xgbe_desc_if *desc_if = &pdata->desc_if;
	struct xgbe_channel *channel;
	struct xgbe_ring *ring;
	struct xgbe_packet_data *packet;
	struct netdev_queue *txq;
	unsigned long flags;
	int ret;

	DBGPR("-->xgbe_xmit: skb->len = %d\n", skb->len);

	channel = pdata->channel + skb->queue_mapping;
	txq = netdev_get_tx_queue(netdev, channel->queue_index);
	ring = channel->tx_ring;
	packet = &ring->packet_data;

	ret = NETDEV_TX_OK;

	spin_lock_irqsave(&ring->lock, flags);

	if (skb->len == 0) {
		netdev_err(netdev, "empty skb received from stack\n");
		dev_kfree_skb_any(skb);
		pstats->txdropped += 1;
		goto tx_netdev_return;
	}

	pstats->txoctetcount += skb->len;
	pstats->txframecount += 1;

	/* Calculate preliminary packet info */
	memset(packet, 0, sizeof(*packet));
	xgbe_packet_info(pdata, ring, skb, packet);

	/* Check that there are enough descriptors available */
	ret = xgbe_maybe_stop_tx_queue(channel, ring, packet->rdesc_count);
	if (ret)
		goto tx_netdev_return;

	ret = xgbe_prep_tso(skb, packet);
	if (ret) {
		netdev_err(netdev, "error processing TSO packet\n");
		dev_kfree_skb_any(skb);
		pstats->txerrorcount += 1;
		goto tx_netdev_return;
	}
	xgbe_prep_vlan(skb, packet);

	if (!desc_if->map_tx_skb(channel, skb)) {
		dev_kfree_skb_any(skb);
		pstats->txerrorcount += 1;
		goto tx_netdev_return;
	}

	xgbe_prep_tx_tstamp(pdata, skb, packet);

	/* Report on the actual number of bytes (to be) sent */
	netdev_tx_sent_queue(txq, packet->tx_bytes);

	/* Configure required descriptor fields for transmission */
	hw_if->dev_xmit(channel);

#ifdef XGMAC_ENABLE_TX_PKT_DUMP
	xgbe_print_pkt(netdev, skb, true);
#endif


	/* Stop the queue in advance if there may not be enough descriptors */
	xgbe_maybe_stop_tx_queue(channel, ring, XGBE_TX_MAX_DESCS);

	ret = NETDEV_TX_OK;

tx_netdev_return:
	spin_unlock_irqrestore(&ring->lock, flags);

	DBGPR("<--xgbe_xmit ret=%x  OK=%x\n", ret, NETDEV_TX_OK);

	return ret;
}

static void xgbe_set_rx_mode(struct net_device *netdev)
{
	struct xgbe_prv_data *pdata = netdev_priv(netdev);
	struct xgbe_hw_if *hw_if = &pdata->hw_if;
	unsigned int pr_mode, am_mode;

	DBGPR("-->xgbe_set_rx_mode\n");

	pr_mode = ((netdev->flags & IFF_PROMISC) != 0);
	am_mode = ((netdev->flags & IFF_ALLMULTI) != 0);

	hw_if->set_promiscuous_mode(pdata, pr_mode);
	hw_if->set_all_multicast_mode(pdata, am_mode);

	hw_if->add_mac_addresses(pdata);

	DBGPR("<--xgbe_set_rx_mode\n");
}

static int xgbe_set_mac_address(struct net_device *netdev, void *addr)
{
	struct xgbe_prv_data *pdata = netdev_priv(netdev);
	struct xgbe_hw_if *hw_if = &pdata->hw_if;
	struct sockaddr *saddr = addr;

	DBGPR("-->xgbe_set_mac_address\n");

	if (!is_valid_ether_addr(saddr->sa_data))
		return -EADDRNOTAVAIL;

	memcpy(netdev->dev_addr, saddr->sa_data, netdev->addr_len);

	hw_if->set_mac_address(pdata, netdev->dev_addr);

	DBGPR("<--xgbe_set_mac_address\n");

	return 0;
}

static int xgbe_ioctl(struct net_device *netdev, struct ifreq *ifreq, int cmd)
{
	struct xgbe_prv_data *pdata = netdev_priv(netdev);
	int ret;

	switch (cmd) {
	case SIOCGHWTSTAMP:
		ret = xgbe_get_hwtstamp_settings(pdata, ifreq);
		break;

	case SIOCSHWTSTAMP:
		ret = xgbe_set_hwtstamp_settings(pdata, ifreq);
		break;

	default:
		ret = -EOPNOTSUPP;
	}

	return ret;
}

static int xgbe_change_mtu(struct net_device *netdev, int mtu)
{
	struct xgbe_prv_data *pdata = netdev_priv(netdev);
	int ret;

	DBGPR("-->xgbe_change_mtu\n");

	ret = xgbe_calc_rx_buf_size(netdev, mtu);
	if (ret < 0)
		return ret;

	pdata->rx_buf_size = ret;
	netdev->mtu = mtu;

	xgbe_restart_dev(pdata, 0);

	DBGPR("<--xgbe_change_mtu\n");

	return 0;
}

static struct rtnl_link_stats64 *xgbe_get_stats64(struct net_device *netdev,
						  struct rtnl_link_stats64 *s)
{
	struct xgbe_prv_data *pdata = netdev_priv(netdev);
	struct xgbe_stats *pstats = &pdata->stats;

	DBGPR("-->%s\n", __func__);

	s->rx_packets = pstats->rxframecount;
	s->rx_bytes = pstats->rxoctetcount;
	s->rx_errors = pstats->rxrunerror;

	s->multicast = pstats->rxmulticastframes;
	s->rx_length_errors = pstats->rxlengtherror;
	s->rx_crc_errors = pstats->rxcrcerror;
	s->rx_fifo_errors = pstats->rxfifooverflow;

	s->tx_packets = pstats->txframecount;
	s->tx_bytes = pstats->txoctetcount;
	s->tx_errors = pstats->txerrorcount;

	DBGPR("<--%s\n", __func__);

	return s;
}

static int xgbe_vlan_rx_add_vid(struct net_device *netdev, __be16 proto,
				u16 vid)
{
	struct xgbe_prv_data *pdata = netdev_priv(netdev);
	struct xgbe_hw_if *hw_if = &pdata->hw_if;

	DBGPR("-->%s\n", __func__);

	set_bit(vid, pdata->active_vlans);
	hw_if->update_vlan_hash_table(pdata);

	DBGPR("<--%s\n", __func__);

	return 0;
}

static int xgbe_vlan_rx_kill_vid(struct net_device *netdev, __be16 proto,
				 u16 vid)
{
	struct xgbe_prv_data *pdata = netdev_priv(netdev);
	struct xgbe_hw_if *hw_if = &pdata->hw_if;

	DBGPR("-->%s\n", __func__);

	clear_bit(vid, pdata->active_vlans);
	hw_if->update_vlan_hash_table(pdata);

	DBGPR("<--%s\n", __func__);

	return 0;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
static void xgbe_poll_controller(struct net_device *netdev)
{
	struct xgbe_prv_data *pdata = netdev_priv(netdev);
	struct xgbe_channel *channel;
	unsigned int i;

	DBGPR("-->xgbe_poll_controller\n");

	if (pdata->per_channel_irq) {
		channel = pdata->channel;
		for (i = 0; i < pdata->channel_count; i++, channel++)
			xgbe_dma_isr(channel->dma_irq, channel);
	} else {
		disable_irq(pdata->dev_irq);
		xgbe_isr(pdata->dev_irq, pdata);
		enable_irq(pdata->dev_irq);
	}

	DBGPR("<--xgbe_poll_controller\n");
}
#endif /* End CONFIG_NET_POLL_CONTROLLER */

static int xgbe_setup_tc(struct net_device *netdev, u8 tc)
{
	struct xgbe_prv_data *pdata = netdev_priv(netdev);
	unsigned int offset, queue;
	u8 i;

	if (tc && (tc != pdata->hw_feat.tc_cnt))
		return -EINVAL;

	if (tc) {
		netdev_set_num_tc(netdev, tc);
		for (i = 0, queue = 0, offset = 0; i < tc; i++) {
			while ((queue < pdata->tx_q_count) &&
			       (pdata->q2tc_map[queue] == i))
				queue++;

			DBGPR("  TC%u using TXq%u-%u\n", i, offset, queue - 1);
			netdev_set_tc_queue(netdev, i, queue - offset, offset);
			offset = queue;
		}
	} else {
		netdev_reset_tc(netdev);
	}

	return 0;
}

static int xgbe_set_features(struct net_device *netdev,
			     netdev_features_t features)
{
	struct xgbe_prv_data *pdata = netdev_priv(netdev);
	struct xgbe_hw_if *hw_if = &pdata->hw_if;
	netdev_features_t rxhash, rxcsum, rxvlan, rxvlan_filter;
	int ret = 0;

	rxhash = pdata->netdev_features & NETIF_F_RXHASH;
	rxcsum = pdata->netdev_features & NETIF_F_RXCSUM;
	rxvlan = pdata->netdev_features & NETIF_F_HW_VLAN_CTAG_RX;
	rxvlan_filter = pdata->netdev_features & NETIF_F_HW_VLAN_CTAG_FILTER;

	if ((features & NETIF_F_RXHASH) && !rxhash)
		ret = hw_if->enable_rss(pdata);
	else if (!(features & NETIF_F_RXHASH) && rxhash)
		ret = hw_if->disable_rss(pdata);
	if (ret)
		return ret;

	if ((features & NETIF_F_RXCSUM) && !rxcsum)
		hw_if->enable_rx_csum(pdata);
	else if (!(features & NETIF_F_RXCSUM) && rxcsum)
		hw_if->disable_rx_csum(pdata);

	if ((features & NETIF_F_HW_VLAN_CTAG_RX) && !rxvlan)
		hw_if->enable_rx_vlan_stripping(pdata);
	else if (!(features & NETIF_F_HW_VLAN_CTAG_RX) && rxvlan)
		hw_if->disable_rx_vlan_stripping(pdata);

	if ((features & NETIF_F_HW_VLAN_CTAG_FILTER) && !rxvlan_filter)
		hw_if->enable_rx_vlan_filtering(pdata);
	else if (!(features & NETIF_F_HW_VLAN_CTAG_FILTER) && rxvlan_filter)
		hw_if->disable_rx_vlan_filtering(pdata);

	pdata->netdev_features = features;

	DBGPR("<--xgbe_set_features\n");

	return 0;
}

static const struct net_device_ops xgbe_netdev_ops = {
	.ndo_open		= xgbe_open,
	.ndo_stop		= xgbe_close,
	.ndo_start_xmit		= xgbe_xmit,
	.ndo_set_rx_mode	= xgbe_set_rx_mode,
	.ndo_set_mac_address	= xgbe_set_mac_address,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_do_ioctl		= xgbe_ioctl,
	.ndo_change_mtu		= xgbe_change_mtu,
	.ndo_get_stats64	= xgbe_get_stats64,
	.ndo_vlan_rx_add_vid	= xgbe_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid	= xgbe_vlan_rx_kill_vid,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller	= xgbe_poll_controller,
#endif
	.ndo_setup_tc		= xgbe_setup_tc,
	.ndo_set_features	= xgbe_set_features,
};

struct net_device_ops *xgbe_get_netdev_ops(void)
{
	return (struct net_device_ops *)&xgbe_netdev_ops;
}

static void xgbe_rx_refresh(struct xgbe_channel *channel)
{
	struct xgbe_prv_data *pdata = channel->pdata;
	struct xgbe_desc_if *desc_if = &pdata->desc_if;
	struct xgbe_ring *ring = channel->rx_ring;
	struct xgbe_ring_data *rdata;

	desc_if->realloc_rx_buffer(channel);

	/* Update the Rx Tail Pointer Register with address of
	 * the last cleaned entry */
	rdata = XGBE_GET_DESC_DATA(ring, ring->rx.realloc_index - 1);
	XGMAC_DMA_IOWRITE(channel, DMA_CH_RDTR_LO,
			  lower_32_bits(rdata->rdesc_dma));
}

static struct sk_buff *xgbe_create_skb(struct xgbe_prv_data *pdata,
				       struct xgbe_ring_data *rdata,
				       unsigned int *len)
{
	struct net_device *netdev = pdata->netdev;
	struct sk_buff *skb;
	u8 *packet;
	unsigned int copy_len;

	skb = netdev_alloc_skb_ip_align(netdev, rdata->rx.hdr.dma_len);
	if (!skb)
		return NULL;

	packet = page_address(rdata->rx.hdr.pa.pages) +
		 rdata->rx.hdr.pa.pages_offset;
	copy_len = (rdata->rx.hdr_len) ? rdata->rx.hdr_len : *len;
	copy_len = min(rdata->rx.hdr.dma_len, copy_len);
	skb_copy_to_linear_data(skb, packet, copy_len);
	skb_put(skb, copy_len);

	*len -= copy_len;

	return skb;
}

static int xgbe_tx_poll(struct xgbe_channel *channel)
{
	struct xgbe_prv_data *pdata = channel->pdata;
	struct xgbe_hw_if *hw_if = &pdata->hw_if;
	struct xgbe_desc_if *desc_if = &pdata->desc_if;
	struct xgbe_ring *ring = channel->tx_ring;
	struct xgbe_ring_data *rdata;
	struct xgbe_ring_desc *rdesc;
	struct net_device *netdev = pdata->netdev;
	struct netdev_queue *txq;
	unsigned long flags;
	int processed = 0;
	unsigned int tx_packets = 0, tx_bytes = 0;

	DBGPR("-->xgbe_tx_poll\n");

	/* Nothing to do if there isn't a Tx ring for this channel */
	if (!ring)
		return 0;

	txq = netdev_get_tx_queue(netdev, channel->queue_index);

	spin_lock_irqsave(&ring->lock, flags);

	while ((processed < XGBE_TX_DESC_MAX_PROC) &&
	       (ring->dirty != ring->cur)) {
		rdata = XGBE_GET_DESC_DATA(ring, ring->dirty);
		rdesc = rdata->rdesc;

		if (!hw_if->tx_complete(rdesc))
			break;

		/* Make sure descriptor fields are read after reading the OWN
		 * bit */
		rmb();

#ifdef XGMAC_ENABLE_TX_DESC_DUMP
		xgbe_dump_tx_desc(ring, ring->dirty, 1, 0);
#endif

		if (hw_if->is_last_desc(rdesc)) {
			tx_packets += rdata->tx.packets;
			tx_bytes += rdata->tx.bytes;
		}

		/* Free the SKB and reset the descriptor for re-use */
		desc_if->unmap_rdata(pdata, rdata);
		hw_if->tx_desc_reset(rdata);

		processed++;
		ring->dirty++;
	}

	if (!processed)
		goto unlock;

	netdev_tx_completed_queue(txq, tx_packets, tx_bytes);

	if ((ring->tx.queue_stopped == 1) &&
	    (xgbe_tx_avail_desc(ring) > XGBE_TX_DESC_MIN_FREE)) {
		ring->tx.queue_stopped = 0;
		netif_tx_wake_queue(txq);
	}

	DBGPR("<--xgbe_tx_poll: processed=%d\n", processed);

unlock:
	spin_unlock_irqrestore(&ring->lock, flags);

	return processed;
}

static int xgbe_rx_poll(struct xgbe_channel *channel, int budget)
{
	struct xgbe_prv_data *pdata = channel->pdata;
	struct xgbe_stats *pstats = &pdata->stats;
	struct xgbe_hw_if *hw_if = &pdata->hw_if;
	struct xgbe_ring *ring = channel->rx_ring;
	struct xgbe_ring_data *rdata;
	struct xgbe_packet_data *packet;
	struct net_device *netdev = pdata->netdev;
	struct napi_struct *napi;
	struct sk_buff *skb;
	struct skb_shared_hwtstamps *hwtstamps;
	unsigned int incomplete, error, context_next, context;
	unsigned int len, put_len, max_len;
	unsigned int received = 0;
	int packet_count = 0;

	DBGPR("-->xgbe_rx_poll: budget=%d\n", budget);

	/* Nothing to do if there isn't a Rx ring for this channel */
	if (!ring)
		return 0;

	napi = (pdata->per_channel_irq) ? &channel->napi : &pdata->napi;

	rdata = XGBE_GET_DESC_DATA(ring, ring->cur);
	packet = &ring->packet_data;
	while (packet_count < budget) {
		DBGPR("  cur = %d\n", ring->cur);

		/* First time in loop see if we need to restore state */
		if (!received && rdata->state_saved) {
			incomplete = rdata->state.incomplete;
			context_next = rdata->state.context_next;
			skb = rdata->state.skb;
			error = rdata->state.error;
			len = rdata->state.len;
		} else {
			memset(packet, 0, sizeof(*packet));
			incomplete = 0;
			context_next = 0;
			skb = NULL;
			error = 0;
			len = 0;
		}

read_again:
		rdata = XGBE_GET_DESC_DATA(ring, ring->cur);

		if (ring->dirty > (XGBE_RX_DESC_CNT >> 3))
			xgbe_rx_refresh(channel);

		if (hw_if->dev_read(channel))
			break;

		received++;
		ring->cur++;
		ring->dirty++;

		incomplete = XGMAC_GET_BITS(packet->attributes,
					    RX_PACKET_ATTRIBUTES,
					    INCOMPLETE);
		context_next = XGMAC_GET_BITS(packet->attributes,
					      RX_PACKET_ATTRIBUTES,
					      CONTEXT_NEXT);
		context = XGMAC_GET_BITS(packet->attributes,
					 RX_PACKET_ATTRIBUTES,
					 CONTEXT);

		pr_info("incomplete=%d  context_next=%d  context=%d\n", incomplete, context_next, context);

		/* Earlier error, just drain the remaining data */
		if ((incomplete || context_next) && error)
			goto read_again;

		if (error || packet->errors) {
			if (packet->errors)
				DBGPR("Error in received packet\n");
			pr_info("error=%d   packet_error=%d\n", error, packet->errors);
			pstats->rxrunerror++;
			if (skb)
				xgbe_print_pkt(netdev, skb, false);
			else
				pr_info("SKB is null!\n");
			dev_kfree_skb(skb);
			goto next_packet;
		}

		if (!context) {
			put_len = rdata->rx.len - len;
			len += put_len;

			if (!skb) {
				dma_sync_single_for_cpu(pdata->dev,
							rdata->rx.hdr.dma,
							rdata->rx.hdr.dma_len,
							DMA_FROM_DEVICE);

				skb = xgbe_create_skb(pdata, rdata, &put_len);
				if (!skb) {
					error = 1;
					goto skip_data;
				}
			}

			if (put_len) {
				dma_sync_single_for_cpu(pdata->dev,
							rdata->rx.buf.dma,
							rdata->rx.buf.dma_len,
							DMA_FROM_DEVICE);

				skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags,
						rdata->rx.buf.pa.pages,
						rdata->rx.buf.pa.pages_offset,
						put_len, rdata->rx.buf.dma_len);
				rdata->rx.buf.pa.pages = NULL;

				pstats->rxoctetcount += skb->len;
				pstats->rxframecount++;
			}
		}

skip_data:
		if (incomplete || context_next)
			goto read_again;

		if (!skb)
			goto next_packet;

		/* Be sure we don't exceed the configured MTU */
		max_len = netdev->mtu + ETH_HLEN;
		if (!(netdev->features & NETIF_F_HW_VLAN_CTAG_RX) &&
		    (skb->protocol == htons(ETH_P_8021Q)))
			max_len += VLAN_HLEN;

		if (skb->len > max_len) {
			DBGPR("packet length exceeds configured MTU\n");
			pr_info("packet length exceeds configured MTU\n");
			dev_kfree_skb(skb);
			pstats->rxrunerror++;
			goto next_packet;
		}

#ifdef XGMAC_ENABLE_RX_PKT_DUMP
		xgbe_print_pkt(netdev, skb, false);
#endif
		skb_checksum_none_assert(skb);
		if (XGMAC_GET_BITS(packet->attributes,
				   RX_PACKET_ATTRIBUTES, CSUM_DONE))
			skb->ip_summed = CHECKSUM_UNNECESSARY;

		if (XGMAC_GET_BITS(packet->attributes,
				   RX_PACKET_ATTRIBUTES, VLAN_CTAG))
			__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q),
					       packet->vlan_ctag);

		if (XGMAC_GET_BITS(packet->attributes,
				   RX_PACKET_ATTRIBUTES, RX_TSTAMP)) {
			u64 nsec;

			nsec = timecounter_cyc2time(&pdata->tstamp_tc,
						    packet->rx_tstamp);
			hwtstamps = skb_hwtstamps(skb);
			hwtstamps->hwtstamp = ns_to_ktime(nsec);
		}

		if (XGMAC_GET_BITS(packet->attributes,
				   RX_PACKET_ATTRIBUTES, RSS_HASH))
			skb_set_hash(skb, packet->rss_hash,
				     packet->rss_hash_type);

		skb->dev = netdev;
		skb->protocol = eth_type_trans(skb, netdev);
		skb_record_rx_queue(skb, channel->queue_index);
		skb_mark_napi_id(skb, napi);

		netdev->last_rx = jiffies;
		napi_gro_receive(napi, skb);

next_packet:
		packet_count++;
	}

	/* Check if we need to save state before leaving */
	if (received && (incomplete || context_next)) {
		rdata = XGBE_GET_DESC_DATA(ring, ring->cur);
		rdata->state_saved = 1;
		rdata->state.incomplete = incomplete;
		rdata->state.context_next = context_next;
		rdata->state.skb = skb;
		rdata->state.len = len;
		rdata->state.error = error;
	}

	DBGPR("<--xgbe_rx_poll: packet_count = %d\n", packet_count);

	return packet_count;
}

static int xgbe_one_poll(struct napi_struct *napi, int budget)
{
	struct xgbe_channel *channel = container_of(napi, struct xgbe_channel,
						    napi);
	int processed = 0;

	DBGPR("-->xgbe_one_poll: budget=%d\n", budget);

	/* Cleanup Tx ring first */
	xgbe_tx_poll(channel);

	/* Process Rx ring next */
	processed = xgbe_rx_poll(channel, budget);

	/* If we processed everything, we are done */
	if (processed < budget) {
		/* Turn off polling */
		napi_complete(napi);

		/* Enable Tx and Rx interrupts */
		enable_irq(channel->dma_irq);
	}

	DBGPR("<--xgbe_one_poll: received = %d\n", processed);

	return processed;
}

static int xgbe_all_poll(struct napi_struct *napi, int budget)
{
	struct xgbe_prv_data *pdata = container_of(napi, struct xgbe_prv_data,
						   napi);
	struct xgbe_channel *channel;
	int ring_budget;
	int processed, last_processed;
	unsigned int i;

	DBGPR("-->xgbe_all_poll: budget=%d\n", budget);

	processed = 0;
	ring_budget = budget / pdata->rx_ring_count;
	do {
		last_processed = processed;

		channel = pdata->channel;
		for (i = 0; i < pdata->channel_count; i++, channel++) {
			/* Cleanup Tx ring first */
			xgbe_tx_poll(channel);

			/* Process Rx ring next */
			if (ring_budget > (budget - processed))
				ring_budget = budget - processed;
			processed += xgbe_rx_poll(channel, ring_budget);
		}
	} while ((processed < budget) && (processed != last_processed));

	/* If we processed everything, we are done */
	if (processed < budget) {
		/* Turn off polling */
		napi_complete(napi);

		/* Enable Tx and Rx interrupts */
		xgbe_enable_rx_tx_ints(pdata);
	}

	DBGPR("<--xgbe_all_poll: received = %d\n", processed);

	return processed;
}

void xgbe_dump_tx_desc(struct xgbe_ring *ring, unsigned int idx,
		       unsigned int count, unsigned int flag)
{
	struct xgbe_ring_data *rdata;
	struct xgbe_ring_desc *rdesc;

	while (count--) {
		rdata = XGBE_GET_DESC_DATA(ring, idx);
		rdesc = rdata->rdesc;
		pr_alert("TX_NORMAL_DESC[%d %s] = %08x:%08x:%08x:%08x\n", idx,
			 (flag == 1) ? "QUEUED FOR TX" : "TX BY DEVICE",
			 le32_to_cpu(rdesc->desc0), le32_to_cpu(rdesc->desc1),
			 le32_to_cpu(rdesc->desc2), le32_to_cpu(rdesc->desc3));
		idx++;
	}
}

void xgbe_dump_rx_desc(struct xgbe_ring *ring, struct xgbe_ring_desc *desc,
		       unsigned int idx)
{
	pr_alert("RX_NORMAL_DESC[%d RX BY DEVICE] = %08x:%08x:%08x:%08x\n", idx,
		 le32_to_cpu(desc->desc0), le32_to_cpu(desc->desc1),
		 le32_to_cpu(desc->desc2), le32_to_cpu(desc->desc3));
}

void xgbe_print_pkt(struct net_device *netdev, struct sk_buff *skb, bool tx_rx)
{
	struct ethhdr *eth = (struct ethhdr *)skb->data;
	unsigned char *buf = skb->data;
	unsigned char buffer[128];
	unsigned int i, j;

	netdev_alert(netdev, "\n************** SKB dump ****************\n");

	netdev_alert(netdev, "%s packet of %d bytes\n",
		     (tx_rx ? "TX" : "RX"), skb->len);

	netdev_alert(netdev, "Dst MAC addr: %pM\n", eth->h_dest);
	netdev_alert(netdev, "Src MAC addr: %pM\n", eth->h_source);
	netdev_alert(netdev, "Protocol: 0x%04hx\n", ntohs(eth->h_proto));

	for (i = 0, j = 0; i < skb->len;) {
		j += snprintf(buffer + j, sizeof(buffer) - j, "%02hhx",
			      buf[i++]);

		if ((i % 32) == 0) {
			netdev_alert(netdev, "  0x%04x: %s\n", i - 32, buffer);
			j = 0;
		} else if ((i % 16) == 0) {
			buffer[j++] = ' ';
			buffer[j++] = ' ';
		} else if ((i % 4) == 0) {
			buffer[j++] = ' ';
		}
	}
	if (i % 32)
		netdev_alert(netdev, "  0x%04x: %s\n", i - (i % 32), buffer);

	netdev_alert(netdev, "\n************** SKB dump ****************\n");
}
