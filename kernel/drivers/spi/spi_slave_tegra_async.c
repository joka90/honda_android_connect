/*
 * Driver for Nvidia TEGRA spi controller in slave async mode.
 *
 * Copyright (c) 2012-2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*#define DEBUG           1*/
/*#define VERBOSE_DEBUG   1*/

#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/gpio.h>

#include <linux/spi/spi.h>
#include <linux/spi-tegra.h>

#include <mach/dma.h>
#include <mach/clk.h>
#include <mach/spi.h>

#define SLINK_COMMAND		0x000
#define   SLINK_BIT_LENGTH(x)		(((x) & 0x1f) << 0)
#define   SLINK_WORD_SIZE(x)		(((x) & 0x1f) << 5)
#define   SLINK_BOTH_EN			(1 << 10)
#define   SLINK_CS_SW			(1 << 11)
#define   SLINK_CS_VALUE		(1 << 12)
#define   SLINK_CS_POLARITY		(1 << 13)
#define   SLINK_IDLE_SDA_DRIVE_LOW	(0 << 16)
#define   SLINK_IDLE_SDA_DRIVE_HIGH	(1 << 16)
#define   SLINK_IDLE_SDA_PULL_LOW	(2 << 16)
#define   SLINK_IDLE_SDA_PULL_HIGH	(3 << 16)
#define   SLINK_IDLE_SDA_MASK		(3 << 16)
#define   SLINK_CS_POLARITY1		(1 << 20)
#define   SLINK_CK_SDA			(1 << 21)
#define   SLINK_CS_POLARITY2		(1 << 22)
#define   SLINK_CS_POLARITY3		(1 << 23)
#define   SLINK_IDLE_SCLK_DRIVE_LOW	(0 << 24)
#define   SLINK_IDLE_SCLK_DRIVE_HIGH	(1 << 24)
#define   SLINK_IDLE_SCLK_PULL_LOW	(2 << 24)
#define   SLINK_IDLE_SCLK_PULL_HIGH	(3 << 24)
#define   SLINK_IDLE_SCLK_MASK		(3 << 24)
#define   SLINK_M_S			(1 << 28)
#define   SLINK_WAIT			(1 << 29)
#define   SLINK_GO			(1 << 30)
#define   SLINK_ENB			(1 << 31)

#define SLINK_COMMAND2		0x004
#define   SLINK_LSBFE			(1 << 0)
#define   SLINK_SSOE			(1 << 1)
#define   SLINK_SPIE			(1 << 4)
#define   SLINK_BIDIROE			(1 << 6)
#define   SLINK_MODFEN			(1 << 7)
#define   SLINK_INT_SIZE(x)		(((x) & 0x1f) << 8)
#define   SLINK_CS_ACTIVE_BETWEEN	(1 << 17)
#define   SLINK_SS_EN_CS(x)		(((x) & 0x3) << 18)
#define   SLINK_SS_SETUP(x)		(((x) & 0x3) << 20)
#define   SLINK_FIFO_REFILLS_0		(0 << 22)
#define   SLINK_FIFO_REFILLS_1		(1 << 22)
#define   SLINK_FIFO_REFILLS_2		(2 << 22)
#define   SLINK_FIFO_REFILLS_3		(3 << 22)
#define   SLINK_FIFO_REFILLS_MASK	(3 << 22)
#define   SLINK_WAIT_PACK_INT(x)	(((x) & 0x7) << 26)
#define   SLINK_SPC0			(1 << 29)
#define   SLINK_TXEN			(1 << 30)
#define   SLINK_RXEN			(1 << 31)

#define SLINK_STATUS		0x008
#define   SLINK_COUNT(val)		(((val) >> 0) & 0x1f)
#define   SLINK_WORD(val)		(((val) >> 5) & 0x1f)
#define   SLINK_BLK_CNT(val)		(((val) >> 0) & 0xffff)
#define   SLINK_MODF			(1 << 16)
#define   SLINK_RX_UNF			(1 << 18)
#define   SLINK_TX_OVF			(1 << 19)
#define   SLINK_TX_FULL			(1 << 20)
#define   SLINK_TX_EMPTY		(1 << 21)
#define   SLINK_RX_FULL			(1 << 22)
#define   SLINK_RX_EMPTY		(1 << 23)
#define   SLINK_TX_UNF			(1 << 24)
#define   SLINK_RX_OVF			(1 << 25)
#define   SLINK_TX_FLUSH		(1 << 26)
#define   SLINK_RX_FLUSH		(1 << 27)
#define   SLINK_SCLK			(1 << 28)
#define   SLINK_ERR			(1 << 29)
#define   SLINK_RDY			(1 << 30)
#define   SLINK_BSY			(1 << 31)

#define SLINK_MAS_DATA		0x010
#define SLINK_SLAVE_DATA	0x014

#define SLINK_DMA_CTL		0x018
#define   SLINK_DMA_BLOCK_SIZE(x)	(((x) & 0xffff) << 0)
#define   SLINK_TX_TRIG_1		(0 << 16)
#define   SLINK_TX_TRIG_4		(1 << 16)
#define   SLINK_TX_TRIG_8		(2 << 16)
#define   SLINK_TX_TRIG_16		(3 << 16)
#define   SLINK_TX_TRIG_MASK		(3 << 16)
#define   SLINK_RX_TRIG_1		(0 << 18)
#define   SLINK_RX_TRIG_4		(1 << 18)
#define   SLINK_RX_TRIG_8		(2 << 18)
#define   SLINK_RX_TRIG_16		(3 << 18)
#define   SLINK_RX_TRIG_MASK		(3 << 18)
#define   SLINK_PACKED			(1 << 20)
#define   SLINK_PACK_SIZE_4		(0 << 21)
#define   SLINK_PACK_SIZE_8		(1 << 21)
#define   SLINK_PACK_SIZE_16		(2 << 21)
#define   SLINK_PACK_SIZE_32		(3 << 21)
#define   SLINK_PACK_SIZE_MASK		(3 << 21)
#define   SLINK_IE_TXC			(1 << 26)
#define   SLINK_IE_RXC			(1 << 27)
#define   SLINK_DMA_EN			(1 << 31)

#define SLINK_STATUS2		0x01c
#define   SLINK_TX_FIFO_EMPTY_COUNT(val)	(((val) & 0x3f) >> 0)
#define   SLINK_RX_FIFO_FULL_COUNT(val)		(((val) & 0x3f0000) >> 16)
#define   SLINK_SS_HOLD_TIME(val)		(((val) & 0xF) << 6)

#define SLINK_TX_FIFO		0x100
#define SLINK_RX_FIFO		0x180

#define DATA_DIR_TX		(1 << 0)
#define DATA_DIR_RX		(1 << 1)

#define SPI_FIFO_DEPTH		32

/* Slave controller clock should be 4 times of the interface clock. However,
 * it is recommended to keep controller clock 8 times of interface clock to
 * avoid some timing issue.
 * */
#define CONTROLLER_SPEED_MULTIPLIER     8

#define SPISLAVE_DRIVER_VER "1.9ro"

static const unsigned long spi_tegra_req_sels[] = {
	TEGRA_DMA_REQ_SEL_SL2B1,
	TEGRA_DMA_REQ_SEL_SL2B2,
	TEGRA_DMA_REQ_SEL_SL2B3,
	TEGRA_DMA_REQ_SEL_SL2B4,
#ifndef CONFIG_ARCH_TEGRA_2x_SOC
	TEGRA_DMA_REQ_SEL_SL2B5,
	TEGRA_DMA_REQ_SEL_SL2B6,
#endif

};

#ifndef CONFIG_SPI_SLAVE_ASYNC_MSGSZ
#error CONFIG_SPI_SLAVE_ASYNC_MSGSZ is undefined
#endif

#define TEGRA_SPI_SLAVE_MAXMSG_SZ CONFIG_SPI_SLAVE_ASYNC_MSGSZ
/* MAX_PACKETS number of max packets the driver should buffer */
#define MAX_PACKETS			16
#define SPI_SLAVE_QUEUE_SZ		(MAX_PACKETS*TEGRA_SPI_SLAVE_MAXMSG_SZ)

#define TX_FIFO_EMPTY_COUNT_MAX		SLINK_TX_FIFO_EMPTY_COUNT(0x20)
#define RX_FIFO_FULL_COUNT_ZERO		SLINK_RX_FIFO_FULL_COUNT(0)

#define SLINK_STATUS2_RESET \
	(TX_FIFO_EMPTY_COUNT_MAX | \
	RX_FIFO_FULL_COUNT_ZERO << 16)

#define MAX_CHIP_SELECT		4
#define SLINK_FIFO_DEPTH	4

struct buffq {
	uint32_t *buff;
	size_t sz;
	size_t fsz;		/* for hole management */
	uint32_t head;
	uint32_t tail;
	spinlock_t lock;
};

struct spi_tegra_data {
	struct spi_master *master;
	struct platform_device *pdev;
	spinlock_t lock;
	char port_name[32];

	struct clk *clk;
	void __iomem *base;
	unsigned long phys;

	u32 cur_speed;

	struct list_head queue;
	unsigned words_per_32bit;
	unsigned bytes_per_word;
	unsigned curr_dma_words;

	struct tegra_dma_req rx_dma_req;
	struct tegra_dma_channel *rx_dma;
	u32 *rx_buf;
	dma_addr_t rx_buf_phys;

	unsigned dma_buf_size;

	bool is_suspended;

	u32 rx_status;
	u32 status_reg;

	u32 command_reg;
	u32 command2_reg;
	u32 dma_control_reg;
	u32 def_command_reg;
	u32 def_command2_reg;

	callback client_slave_ready_cb;
	void *client_data;
	int cs_gpio;
	bool enabled;
	struct buffq q;
	u32 max_len;
	struct completion data_ready;
};

#define sizefilled(q) ((q)->tail >= (q)->head ? (q)->tail - (q)->head : \
			(q)->sz - ((q)->head-(q)->tail)+1)
#define sizeleft(q) ((q)->tail < (q)->head ? (q)->head - (q)->tail - 1 : \
			(q)->sz - ((q)->tail - (q)->head))

#define cadd(_p, _v, _q) (((_p) + (_v))%((_q)->sz + 1))

void memcpy_unpacked(uint8_t *dest, const uint32_t *unpacked, size_t size)
{
	int i;
	for (i = 0; i < size; i++)
		*dest++ = *(char *)unpacked++;
}

uint32_t *q_enq_getbuff(struct buffq *q, size_t sz)
{
	uint32_t free;
	unsigned long flags;
	uint32_t *buff = NULL;

	spin_lock_irqsave(&q->lock, flags);

	/* reset the hole if any */
	if (q->head <= q->tail)
		q->sz = q->fsz;

	/* check for space */
	free = sizeleft(q);
	BUG_ON(free > q->sz);

	if (free < sz) {
		spin_unlock_irqrestore(&q->lock, flags);
		return NULL;
	}

	if (q->tail + sz <= q->sz + 1) {
		/* try to fit from tail to end */
		buff = q->buff + q->tail;
	} else if (q->head > sz) {
		/* try to fit from start to head */
		/* we have a hole here so reset size at start of hole */
		q->sz = q->tail - 1;
		q->tail = 0;
		buff = q->buff + q->tail;
	} else {
		buff = NULL;
	}

	spin_unlock_irqrestore(&q->lock, flags);
	return buff;
}

void q_enq_complete(struct buffq *q, size_t sz)
{
	unsigned long flags;
	spin_lock_irqsave(&q->lock, flags);
	BUG_ON(sizeleft(q) < sz);
	q->tail = cadd(q->tail, sz, q);
	spin_unlock_irqrestore(&q->lock, flags);
}

size_t q_deq_unpacked(struct buffq *q, uint8_t * buff, size_t sz)
{
	uint32_t avail, end, head;
	unsigned long flags;

	spin_lock_irqsave(&q->lock, flags);
	avail = sizefilled(q);
	if (!avail) {
		spin_unlock_irqrestore(&q->lock, flags);
		return 0;
	}
	BUG_ON(avail > q->sz);
	sz = min(sz, avail);
	head = q->head;
	if (head + sz <= q->sz + 1) {	/* no wrap */
		spin_unlock_irqrestore(&q->lock, flags);
		memcpy_unpacked(buff, q->buff + head, sz);
	} else {		/* circular copy for wrap */

		end = q->sz - head + 1;
		spin_unlock_irqrestore(&q->lock, flags);
		/* copy from tail to array-end */
		memcpy_unpacked(buff, q->buff + head, end);
		/* copy from 0 to remaining */
		memcpy_unpacked(buff + end, q->buff, sz - end);
	}
	/* Free space updated after memcpy to avaoid race with
	 * enqueue operation */
	spin_lock_irqsave(&q->lock, flags);
	/* Make sure resetq was not called in between */
	if (sizefilled(q) >= avail)
		q->head = cadd(head, sz, q);
	spin_unlock_irqrestore(&q->lock, flags);
	return sz;
}

void resetq(struct buffq *q)
{
	unsigned long flags;
	spin_lock_irqsave(&q->lock, flags);
	q->tail = q->head = 0;
	q->sz = q->fsz;
	spin_unlock_irqrestore(&q->lock, flags);
}

void initq(struct buffq *q, uint32_t * buff, size_t sz)
{
	q->tail = q->head = 0;
	q->buff = buff;
	q->fsz = sz - 1;	/* extra space for head and tail management */
	q->sz = q->fsz;

	spin_lock_init(&q->lock);
}

static inline unsigned long spi_tegra_readl(struct spi_tegra_data *tspi,
					    unsigned long reg)
{
	return readl(tspi->base + reg);
}

static inline void spi_tegra_writel(struct spi_tegra_data *tspi,
				    unsigned long val, unsigned long reg)
{
	writel(val, tspi->base + reg);
}

int spi_tegra_async_register_callback(struct spi_device *spi, callback func,
				      void *client_data)
{
	struct spi_tegra_data *tspi = spi_master_get_devdata(spi->master);

	if (!tspi || !func)
		return -EINVAL;
	tspi->client_slave_ready_cb = func;
	tspi->client_data = client_data;
	return 0;
}
EXPORT_SYMBOL(spi_tegra_async_register_callback);

static void spi_tegra_reset_async(struct spi_tegra_data *tspi)
{
	/* reset the controller */
	tegra_periph_reset_assert(tspi->clk);
	udelay(2);
	tegra_periph_reset_deassert(tspi->clk);

	tspi->enabled = false;
}

static void spi_tegra_stop_controller(struct spi_device *spi)
{
	struct spi_tegra_data *tspi = spi_master_get_devdata(spi->master);
	unsigned long flags;

	spin_lock_irqsave(&tspi->lock, flags);
	spi_tegra_reset_async(tspi);
	spin_unlock_irqrestore(&tspi->lock, flags);
}

static int spi_tegra_enq_rx_dma(struct spi_tegra_data *tspi)
{
	int ret = 0;
	u32 *dma_buff;
	size_t req_len;

	/* remove any exisiting dma receive reqs */
	tegra_dma_dequeue_req(tspi->rx_dma, &tspi->rx_dma_req);

	/* reset request flags */
	tspi->rx_dma_req.bytes_transferred = 0;
	tspi->rx_dma_req.size = 0;

	/*  arm the controller for extra words (at max 8), so that
	 *  1) we always abort the controller
	 *  2) Transfer length is 8 words aligned */
	req_len = tspi->max_len + 8 - (tspi->max_len % 8);

	/* get buffer of max packet size for next dma */
	dma_buff = q_enq_getbuff(&tspi->q, req_len);
	if (!dma_buff)
		return -ENOMEM;

	tspi->rx_dma_req.size = req_len * 4;
	tspi->rx_dma_req.virt_addr = dma_buff;
	tspi->rx_dma_req.dest_addr =
	    tspi->rx_buf_phys + ((uint32_t) dma_buff - (uint32_t) tspi->rx_buf);

	ret = tegra_dma_enqueue_req(tspi->rx_dma, &tspi->rx_dma_req);
	if (ret < 0) {
		dev_err(&tspi->pdev->dev,
			"Error in starting rx dma error = %d\n", ret);
		return ret;
	}
	return ret;
}

static void spi_tegra_start_rx_dma(struct spi_tegra_data *tspi)
{
	uint32_t val;
	val = spi_tegra_readl(tspi, SLINK_DMA_CTL);
	val |= SLINK_DMA_EN;
	spi_tegra_writel(tspi, val, SLINK_DMA_CTL);

	tspi->enabled = true;
	if (tspi->client_slave_ready_cb)
		tspi->client_slave_ready_cb(tspi->client_data);
}

static void spi_tegra_setup_clock(struct spi_device *spi,
				  struct spi_tegra_data *tspi)
{
	u32 speed;
	speed = spi->max_speed_hz;
	if (speed != tspi->cur_speed) {
		clk_set_rate(tspi->clk, speed * CONTROLLER_SPEED_MULTIPLIER);
		tspi->cur_speed = speed;
	}
}

static int spi_tegra_setup_controller(struct spi_device *spi,
				      struct spi_tegra_data *tspi)
{
	int ret;
	unsigned long command, command2, dmactl;
	unsigned int len;
	unsigned long cs_bit, def_command_reg;

	BUG_ON(spi->chip_select >= MAX_CHIP_SELECT);
	switch (spi->chip_select) {
	case 0:
		cs_bit = SLINK_CS_POLARITY;
		break;

	case 1:
		cs_bit = SLINK_CS_POLARITY1;
		break;

	case 2:
		cs_bit = SLINK_CS_POLARITY2;
		break;

	case 3:
		cs_bit = SLINK_CS_POLARITY3;
		break;

	default:
		return -EINVAL;
	}

	def_command_reg = tspi->def_command_reg;
	if (spi->mode & SPI_CS_HIGH)
		def_command_reg |= cs_bit;
	else
		def_command_reg &= ~cs_bit;

	command = def_command_reg;
	command |= SLINK_BIT_LENGTH(spi->bits_per_word - 1);

	command &= ~SLINK_IDLE_SCLK_MASK & ~SLINK_CK_SDA;
	if (spi->mode & SPI_CPHA)
		command |= SLINK_CK_SDA;

	if (spi->mode & SPI_CPOL)
		command |= SLINK_IDLE_SCLK_DRIVE_HIGH;
	else
		command |= SLINK_IDLE_SCLK_DRIVE_LOW;

	/* command2 register */
	command2 = tspi->def_command2_reg;
	command2 &=
	    ~(SLINK_SS_EN_CS(~0) | SLINK_RXEN | SLINK_TXEN | SLINK_LSBFE);
	command2 |= SLINK_RXEN;
	command2 |= SLINK_SS_EN_CS(spi->chip_select);

	if (spi->mode & SPI_LSB_FIRST)
		command2 |= SLINK_LSBFE;

	/* dma register */
	tspi->curr_dma_words = tspi->max_len;
	dmactl = SLINK_DMA_BLOCK_SIZE(tspi->curr_dma_words - 1);
	len = tspi->curr_dma_words * 4;

	if (len & 0xF)
		dmactl |= SLINK_TX_TRIG_1 | SLINK_RX_TRIG_1;
	else if (((len) >> 4) & 0x1)
		dmactl |= SLINK_TX_TRIG_4 | SLINK_RX_TRIG_4;
	else
		dmactl |= SLINK_TX_TRIG_8 | SLINK_RX_TRIG_8;

	dmactl |= SLINK_IE_RXC;
	dmactl &= ~SLINK_PACKED;

	tspi->def_command_reg = def_command_reg;

	spi_tegra_writel(tspi, command, SLINK_COMMAND);
	tspi->command_reg = command;

	spi_tegra_writel(tspi, command2, SLINK_COMMAND2);
	tspi->command2_reg = command2;

	spi_tegra_writel(tspi, dmactl, SLINK_DMA_CTL);
	tspi->dma_control_reg = dmactl;

	dev_dbg(&tspi->pdev->dev, "cmd: %08X cmd2:%08X dma:%08X\n",
		tspi->command_reg, tspi->command2_reg, tspi->dma_control_reg);
	return ret;
}

/* called from spi IOC setup ioctl for each parameter */
static int spi_tegra_setup(struct spi_device *spi)
{
	dev_dbg(&spi->dev, "setup %d bpw, %scpol, %scpha, %dHz\n",
		spi->bits_per_word,
		spi->mode & SPI_CPOL ? "" : "~",
		spi->mode & SPI_CPHA ? "" : "~", spi->max_speed_hz);

	/* we support only 8 bpw */
	if (spi->bits_per_word != 8)
		return -EINVAL;

	/* nothing to do here as the values will be saved
	 * in the spi->.. variables */
	return 0;
}

static int spi_tegra_start_rx(struct spi_device *spi)
{
	struct spi_tegra_data *tspi = spi_master_get_devdata(spi->master);
	unsigned long flags;

	dev_dbg(&spi->dev, "%s: setup %d bpw, %scpol, %scpha, %dHz\n",
		__func__,
		spi->bits_per_word,
		spi->mode & SPI_CPOL ? "" : "~",
		spi->mode & SPI_CPHA ? "" : "~", spi->max_speed_hz);

	spin_lock_irqsave(&tspi->lock, flags);
	spi_tegra_reset_async(tspi);
	spi_tegra_setup_clock(spi, tspi);
	spi_tegra_setup_controller(spi, tspi);
	resetq(&tspi->q);
	/* not checking return value here */
	spi_tegra_enq_rx_dma(tspi);
	spi_tegra_start_rx_dma(tspi);
	spin_unlock_irqrestore(&tspi->lock, flags);
	return 0;
}

/* called from spi IOC transfer ioctl */
static int spi_tegra_transfer(struct spi_device *spi, struct spi_message *m)
{
	struct spi_tegra_data *tspi = spi_master_get_devdata(spi->master);
	struct spi_transfer *t;
	int len, ret;
	unsigned long flags;

	/* Support only one transfer per message */
	if (!list_is_singular(&m->transfers))
		return -EINVAL;

	if (list_empty(&m->transfers) || !m->complete)
		return -EINVAL;

	t = list_first_entry(&m->transfers, struct spi_transfer, transfer_list);
	if (t->bits_per_word != 8)
		return -EINVAL;

	if (t->len == 0 || !t->rx_buf)
		return -EINVAL;

	spin_lock_irqsave(&tspi->lock, flags);
	if (WARN_ON(tspi->is_suspended)) {
		spin_unlock_irqrestore(&tspi->lock, flags);
		return -EBUSY;
	}
	spin_unlock_irqrestore(&tspi->lock, flags);

	len = q_deq_unpacked(&tspi->q, t->rx_buf, t->len);
	/* if no data wait for timeout */
	if (!len && t->delay_usecs) {
		ret =
		    wait_for_completion_interruptible_timeout(&tspi->data_ready,
							      usecs_to_jiffies
							      (t->delay_usecs));
		if (ret > 0)
			len = q_deq_unpacked(&tspi->q, t->rx_buf, t->len);
	}
	if (!len) {
		m->actual_length = 0;
		m->status = -EAGAIN;
		ret = -EAGAIN;
	} else {
		m->actual_length = t->len = ret = len;
		m->status = 0;
	}
	m->complete(m->context);

	return ret;
}

static void tegra_spi_rx_dma_complete(struct tegra_dma_req *req)
{
	/* not handling rx_dma complete as this will be done in
	 * cs-deassert event */

	/* this checks that the dma is always aborted and there
	 * is no complete case */
	BUG_ON(req->status == 0);
}

/* This function will be called on spi master cs deassert event
   * abort dma
   * transfer data from dma and fifo to buffer queue
   * reset the controller
   * re-arm the controller
 */
static irqreturn_t cs_deassert_irq(int irq, void *data)
{
	unsigned long flags;
	unsigned long val;
	unsigned int word_tfr = 0, pending_rx_words, length = 0, prw = 0;
	int err = 0;
	struct spi_tegra_data *tspi = (struct spi_tegra_data *)data;
	u32 *rx_buf;

	spin_lock_irqsave(&tspi->lock, flags);
	/* chip select false toggle ? */
	if (!tspi->enabled) {
		spin_unlock_irqrestore(&tspi->lock, flags);
		return IRQ_HANDLED;
	}
	tspi->status_reg = spi_tegra_readl(tspi, SLINK_STATUS);

	/* disable dma */
	/* is this enough instead to stop controller in all conditions ? */
	val = spi_tegra_readl(tspi, SLINK_DMA_CTL);
	val &= ~SLINK_DMA_EN;
	spi_tegra_writel(tspi, val, SLINK_DMA_CTL);

	tegra_dma_dequeue_req(tspi->rx_dma, &tspi->rx_dma_req);

	if (tspi->status_reg & (SLINK_RX_OVF | SLINK_RX_UNF)) {
		dev_info(&tspi->pdev->dev, "%s: Error in status:0x%x\n",
			 __func__, tspi->status_reg);
		err = -EIO;
		goto reset_and_rearm;
	}

	/* check for dma transferred bytes and read out the remaining from
	 * FIFO */
	rx_buf = tspi->rx_dma_req.virt_addr;
	/* dma transferred size */
	word_tfr = tspi->rx_dma_req.bytes_transferred / 4;
	prw = pending_rx_words =
	    SLINK_RX_FIFO_FULL_COUNT(spi_tegra_readl(tspi, SLINK_STATUS2));

	tspi->rx_dma_req.bytes_transferred += pending_rx_words * 4;
	while (pending_rx_words--)
		rx_buf[word_tfr++] = spi_tegra_readl(tspi, SLINK_RX_FIFO);

	length = tspi->rx_dma_req.bytes_transferred / 4;
	BUG_ON(length > tspi->max_len);

	/* dma happened on the buffer, now update buffer ptr to queue */
	q_enq_complete(&tspi->q, length);

reset_and_rearm:
	/* increment the semaphore only once here
	   The transfer function would try to dequeue as much data available
	   and hence incrementing multiple times will return immediately
	   -EAGAIN for further calls. By incrementing only once for whatever
	   data is available, we ensure that maximum of only one -EAGAIN is
	   returned in case of multiple calls
	 */
	try_wait_for_completion(&tspi->data_ready);
	complete(&tspi->data_ready);

	/* reset controller for next transfer */
	spi_tegra_reset_async(tspi);
	/* start next dma request */
	err = spi_tegra_enq_rx_dma(tspi);
	if (!err) {
		/* reprogram the controller */
		spi_tegra_writel(tspi, tspi->command_reg, SLINK_COMMAND);
		spi_tegra_writel(tspi, tspi->command2_reg, SLINK_COMMAND2);
		spi_tegra_writel(tspi, tspi->dma_control_reg, SLINK_DMA_CTL);

		/* start next transfer */
		spi_tegra_start_rx_dma(tspi);
	} else {		/* else no buffer available */
		/* keep the controller enabled */
		tspi->enabled = true;
	}
	spin_unlock_irqrestore(&tspi->lock, flags);
	return IRQ_HANDLED;
}

static int __init spi_tegra_probe(struct platform_device *pdev)
{
	struct spi_master *master;
	struct spi_tegra_data *tspi;
	struct resource *r;
	struct tegra_spi_platform_data *pdata = pdev->dev.platform_data;
	int ret = 0;

	printk(KERN_INFO"Registering spi slave async driver ver %s on %s\n",
	       SPISLAVE_DRIVER_VER, __DATE__);
	master = spi_alloc_master(&pdev->dev, sizeof *tspi);
	if (master == NULL) {
		dev_err(&pdev->dev, "master allocation failed\n");
		return -ENOMEM;
	}

	/* the spi->mode bits understood by this driver: */
	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH | SPI_LSB_FIRST;

	if (pdev->id != -1)
		master->bus_num = pdev->id;

	master->setup = spi_tegra_setup;
	master->transfer = NULL;
	master->transfer_unlocked = spi_tegra_transfer;
	master->start_controller = spi_tegra_start_rx;
	master->stop_controller = spi_tegra_stop_controller;
	master->num_chipselect = MAX_CHIP_SELECT;

	dev_set_drvdata(&pdev->dev, master);
	tspi = spi_master_get_devdata(master);
	tspi->master = master;
	tspi->pdev = pdev;
	spin_lock_init(&tspi->lock);
	if (pdata) {
		tspi->cs_gpio = pdata->cs_gpio;
	} else {
		dev_err(&pdev->dev, "cs-gpio not specified, aborting\n");
		ret = -EINVAL;
		goto fail_no_mem;
	}

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (r == NULL) {
		ret = -ENODEV;
		goto fail_no_mem;
	}

	if (!request_mem_region(r->start, (r->end - r->start) + 1,
				dev_name(&pdev->dev))) {
		ret = -EBUSY;
		goto fail_no_mem;
	}

	tspi->phys = r->start;
	tspi->base = ioremap(r->start, r->end - r->start + 1);
	if (!tspi->base) {
		dev_err(&pdev->dev, "can't ioremap iomem\n");
		ret = -ENOMEM;
		goto fail_io_map;
	}

	sprintf(tspi->port_name, "tegra_spi_%d", pdev->id);
	tspi->clk = clk_get(&pdev->dev, NULL);
	if (IS_ERR_OR_NULL(tspi->clk)) {
		dev_err(&pdev->dev, "can not get clock\n");
		ret = PTR_ERR(tspi->clk);
		goto fail_clk_get;
	}
	tegra_periph_reset_assert(tspi->clk);
	clk_enable(tspi->clk);
	udelay(2);
	tegra_periph_reset_deassert(tspi->clk);

	INIT_LIST_HEAD(&tspi->queue);

	tspi->dma_buf_size = sizeof(uint32_t) * (SPI_SLAVE_QUEUE_SZ + 1);

	tspi->rx_dma =
	    tegra_dma_allocate_channel(TEGRA_DMA_MODE_ONESHOT,
				       "dma_spi%d_async_rx", pdev->id);
	if (!tspi->rx_dma) {
		dev_err(&pdev->dev, "can not allocate rx dma channel\n");
		ret = -ENODEV;
		goto fail_rx_dma_alloc;
	}

	tspi->rx_buf = dma_alloc_coherent(&pdev->dev, tspi->dma_buf_size,
					  &tspi->rx_buf_phys, GFP_KERNEL);
	if (!tspi->rx_buf) {
		dev_err(&pdev->dev, "can not allocate rx bounce buffer\n");
		ret = -ENOMEM;
		goto fail_rx_buf_alloc;
	}

	initq(&tspi->q, tspi->rx_buf, SPI_SLAVE_QUEUE_SZ + 1);

	memset(&tspi->rx_dma_req, 0, sizeof(struct tegra_dma_req));
	tspi->rx_dma_req.complete = tegra_spi_rx_dma_complete;
	tspi->rx_dma_req.to_memory = 1;
	tspi->rx_dma_req.dest_addr = tspi->rx_buf_phys;
	tspi->rx_dma_req.virt_addr = tspi->rx_buf;
	tspi->rx_dma_req.dest_bus_width = 32;
	tspi->rx_dma_req.source_addr = tspi->phys + SLINK_RX_FIFO;
	tspi->rx_dma_req.source_bus_width = 32;
	tspi->rx_dma_req.source_wrap = 4;
	tspi->rx_dma_req.dest_wrap = 0;
	tspi->rx_dma_req.req_sel = spi_tegra_req_sels[pdev->id];
	tspi->rx_dma_req.dev = tspi;

	tspi->def_command_reg = SLINK_CS_SW;
	tspi->def_command2_reg = SLINK_CS_ACTIVE_BETWEEN;

	/* we do unpacked-8bit-max length transfer */
	tspi->bytes_per_word = 1;
	tspi->words_per_32bit = 1;
	tspi->max_len = TEGRA_SPI_SLAVE_MAXMSG_SZ;

	spi_tegra_writel(tspi, tspi->def_command2_reg, SLINK_COMMAND2);

	ret = gpio_request(tspi->cs_gpio, "spi-cs-gpio");
	if (ret)
		goto fail_gpio_request;

	ret = gpio_direction_input(tspi->cs_gpio);
	if (ret)
		goto fail_cs_irq_register;

	tegra_gpio_enable(tspi->cs_gpio);
	ret =
	    request_irq(gpio_to_irq(tspi->cs_gpio), cs_deassert_irq,
			IRQF_TRIGGER_RISING, "spi-cs-gpio", tspi);
	if (ret)
		goto fail_cs_irq_register;
	dev_info(&pdev->dev, "Registered cs-gpio deassert event.\n");

	init_completion(&tspi->data_ready);
	tspi->enabled = false;
	ret = spi_register_master(master);
	if (ret < 0) {
		dev_err(&pdev->dev, "can not register to master err %d\n", ret);
		goto fail_master_register;
	}

	return ret;

fail_cs_irq_register:
	gpio_free(tspi->cs_gpio);
fail_gpio_request:
fail_master_register:
	if (tspi->rx_buf)
		dma_free_coherent(&pdev->dev, tspi->dma_buf_size,
				  tspi->rx_buf, tspi->rx_buf_phys);
fail_rx_buf_alloc:
	if (tspi->rx_dma)
		tegra_dma_free_channel(tspi->rx_dma);

fail_rx_dma_alloc:
	clk_put(tspi->clk);
fail_clk_get:
	iounmap(tspi->base);
fail_io_map:
	release_mem_region(r->start, (r->end - r->start) + 1);
fail_no_mem:
	spi_master_put(master);
	return ret;
}

static int __devexit spi_tegra_remove(struct platform_device *pdev)
{
	struct spi_master *master;
	struct spi_tegra_data *tspi;
	struct resource *r;

	master = dev_get_drvdata(&pdev->dev);
	tspi = spi_master_get_devdata(master);

	if (tspi->rx_buf)
		dma_free_coherent(&pdev->dev, tspi->dma_buf_size,
				  tspi->rx_buf, tspi->rx_buf_phys);
	if (tspi->rx_dma)
		tegra_dma_free_channel(tspi->rx_dma);

	clk_disable(tspi->clk);

	clk_put(tspi->clk);
	iounmap(tspi->base);

	spi_master_put(master);
	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(r->start, (r->end - r->start) + 1);

	gpio_free(tspi->cs_gpio);
	return 0;
}

#ifdef CONFIG_PM
static int spi_tegra_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct spi_master *master;
	struct spi_tegra_data *tspi;
	unsigned long flags;
	unsigned limit = 50;

	master = dev_get_drvdata(&pdev->dev);
	tspi = spi_master_get_devdata(master);
	spin_lock_irqsave(&tspi->lock, flags);
	tspi->is_suspended = true;

	WARN_ON(!list_empty(&tspi->queue));

	while (!list_empty(&tspi->queue) && limit--) {
		spin_unlock_irqrestore(&tspi->lock, flags);
		msleep(20);
		spin_lock_irqsave(&tspi->lock, flags);
	}

	spin_unlock_irqrestore(&tspi->lock, flags);
	clk_disable(tspi->clk);
	return 0;
}

static int spi_tegra_resume(struct platform_device *pdev)
{
	struct spi_master *master;
	struct spi_tegra_data *tspi;
	unsigned long flags;

	master = dev_get_drvdata(&pdev->dev);
	tspi = spi_master_get_devdata(master);

	spin_lock_irqsave(&tspi->lock, flags);
	clk_enable(tspi->clk);
	spi_tegra_writel(tspi, tspi->command_reg, SLINK_COMMAND);
	clk_disable(tspi->clk);

	tspi->cur_speed = 0;
	tspi->is_suspended = false;
	spin_unlock_irqrestore(&tspi->lock, flags);
	return 0;
}
#endif

MODULE_ALIAS("m:spi_slave_tegra_async");

static struct platform_driver spi_tegra_driver = {
	.driver = {
		   .name = "spi_slave_tegra_async",
		   .owner = THIS_MODULE,
		   },
	.remove = __devexit_p(spi_tegra_remove),
#ifdef CONFIG_PM
	.suspend = spi_tegra_suspend,
	.resume = spi_tegra_resume,
#endif
};

static int __init spi_tegra_init(void)
{
	return platform_driver_probe(&spi_tegra_driver, spi_tegra_probe);
}

subsys_initcall(spi_tegra_init);

static void __exit spi_tegra_exit(void)
{
	platform_driver_unregister(&spi_tegra_driver);
}

module_exit(spi_tegra_exit);

MODULE_LICENSE("GPL");
