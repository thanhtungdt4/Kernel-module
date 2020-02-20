#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/can/dev.h>
#include <linux/can/core.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/can.h>
#include <linux/can/error.h>
#include <linux/can/led.h>

#include "mcp_can_dfs.h"

#define CAN_FRAME_MAX_LEN	8
#define SPI_TRANSFER_LEN	(6 + CAN_FRAME_MAX_LEN)
#define OSC_MCP2515_CLK		16000000

struct mcp2515_can_priv {
	struct can_priv		can;
	struct net_device	*net;
	struct spi_device	*spi;
	struct mutex		lock;
	struct sk_buff		*tx_skb;
	int 			tx_len;
	uint8_t 		*spi_tx_buf;
	uint8_t			*spi_rx_buf;
};

static const struct can_bittiming_const mcp2515_bittiming_const = {
	.name = "mcp2515",
	.tseg1_min = 3,
	.tseg1_max = 16,
	.tseg2_min = 2,
	.tseg2_max = 8,
	.sjw_max = 4,
	.brp_min = 1,
	.brp_max = 64,
	.brp_inc = 1,
};

static netdev_tx_t mcp2515_start_xmit(struct sk_buff *skb,
					struct net_device *net)
{
	struct mcp2515_can_priv *priv = netdev_priv(net);
	struct can_frame *frame = (struct can_frame*)skb->data;

	if (can_dropped_invalid_skb(net, skb))
		return NETDEV_TX_OK;

	netif_stop_queue(net);
	priv->tx_skb = skb;

	mutex_lock(&priv->lock);
	if (skb) {
		if (frame->can_dlc > CAN_MAX_CHAR_IN_MESSAGE)
			frame->can_dlc = CAN_MAX_CHAR_IN_MESSAGE;

		sendMsgBuf(priv->spi, frame->can_id, 0, 0, 8, frame->data, true);
		priv->tx_len = 1 + frame->can_dlc;
		can_put_echo_skb(skb, net, 0);
		priv->tx_skb = NULL;
	}

	mutex_unlock(&priv->lock);

	return NETDEV_TX_OK;
}

static void mcp2515_rx(struct spi_device *spi)
{
	struct mcp2515_can_priv *priv = spi_get_drvdata(spi);
	struct sk_buff *skb;
	struct can_frame *frame;
	uint8_t buf[8];
	uint8_t can_len = 0;

	skb = alloc_can_skb(priv->net, &frame);
	if (!skb) {
		pr_err("Can not allocate RX skb\n");
		priv->net->stats.rx_dropped++;
		return;
	}

	if (CAN_MSGAVAIL == checkReceive(spi)) {
		if (readMsgBuf(spi, &can_len, buf) == CAN_NOMSG) {
			pr_err("No Can message\n");
		}
		frame->can_id = getCanId();
		frame->can_dlc = can_len;
		memcpy(frame->data, buf, can_len);

		priv->net->stats.rx_packets++;
		priv->net->stats.rx_bytes += frame->can_dlc;

		can_led_event(priv->net, CAN_LED_EVENT_RX);

		netif_receive_skb(skb);
	}
}

static irqreturn_t can_irq_handle(int irq, void *data)
{
	struct mcp2515_can_priv *priv = (struct mcp2515_can_priv *)data;
	struct spi_device *spi = priv->spi;
	struct net_device *net = priv->net;
	uint8_t status;

	mutex_lock(&priv->lock);
	status = readRxTxStatus(spi);

	mcp2515_rx(spi);

	/* clear interrupt flag */
	if (status & MCP_RX0IF)
		mcp2515_modifyRegister(spi, MCP_CANINTF, 0x01, 0x00);
	else if (status & MCP_RX1IF)
		mcp2515_modifyRegister(spi, MCP_CANINTF, 0x02, 0x00);

	netif_wake_queue(net);

	mutex_unlock(&priv->lock);

	return IRQ_HANDLED;
}

static void mcp2515_clean(struct net_device *net)
{
	struct mcp2515_can_priv *priv = netdev_priv(net);

	if (priv->tx_skb || priv->tx_len)
		net->stats.tx_errors++;
	dev_kfree_skb(priv->tx_skb);

	if (priv->tx_len)
		can_free_echo_skb(priv->net, 0);
	priv->tx_skb = NULL;
	priv->tx_len = 0;
}

static int mcp2515_stop(struct net_device *net)
{
	struct mcp2515_can_priv *priv = netdev_priv(net);
	struct spi_device *spi = priv->spi;

	close_candev(net);
	free_irq(spi->irq, priv);

	mutex_lock(&priv->lock);

	/* Disable and clear pending interrupt */
	mcp2515_setRegister(spi, MCP_CANINTE, 0x00);
	mcp2515_clean(net);
	priv->can.state = CAN_STATE_STOPPED;

	can_led_event(net, CAN_LED_EVENT_STOP);

	mutex_unlock(&priv->lock);

	return 0;
}

static int mcp2515_start(struct net_device *net)
{
	struct mcp2515_can_priv *priv = netdev_priv(net);
	struct spi_device *spi = priv->spi;

	if (can_begin(spi, CAN_100KBPS, MCP_16MHz) == CAN_FAILINIT) {
		pr_err("Can not init CanSPI\n");
		return CAN_FAILINIT;
	}
	else
		pr_info("Can init sucessfully\n");

	return 0;
}

static int mcp2515_do_set_mode(struct net_device *net, enum can_mode mode)
{
	int ret;

	switch (mode) {
	case CAN_MODE_START:
		ret = mcp2515_start(net);
		if (ret)
			return ret;
		netif_wake_queue(net);
		break;

	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static int mcp2515_open(struct net_device *net)
{
	struct mcp2515_can_priv *priv = netdev_priv(net);
	struct spi_device *spi = priv->spi;
	int ret;

	pr_info("Tung: bitrate is %d\n", priv->can.bittiming.bitrate);
	ret = open_candev(net);
	if (ret) {
		pr_err("Unable to set initial baudrate!\n");
		return ret;
	}

	mutex_lock(&priv->lock);
	ret = request_threaded_irq(spi->irq, NULL,
				(irq_handler_t)can_irq_handle,
				IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				"can driver", priv);
	if (ret) {
		pr_err("Tung: Can not request interrupt\n");
		free_irq(spi->irq, priv);
	}

	mcp2515_start(net);
	can_led_event(net, CAN_LED_EVENT_OPEN);

	netif_start_queue(net);
	mutex_unlock(&priv->lock);

	return 0;
}

static const struct net_device_ops mcp2515_net_ops = {
	.ndo_open = mcp2515_open,
	.ndo_stop = mcp2515_stop,
	.ndo_start_xmit = mcp2515_start_xmit,
	.ndo_change_mtu = can_change_mtu,
};

static int dev_probe(struct spi_device *spi)
{
	struct net_device *net;
	struct mcp2515_can_priv *priv;
	int ret;

	pr_info("Tung: Jumpt to probe function\n");
	net = alloc_candev(sizeof(struct mcp2515_can_priv), 1);
	if (!net) 
		return -ENOMEM;
	net->netdev_ops = &mcp2515_net_ops;
	net->flags |= IFF_ECHO;

	priv = netdev_priv(net);
	priv->can.clock.freq = OSC_MCP2515_CLK;
	priv->can.bittiming_const = &mcp2515_bittiming_const;
	priv->can.do_set_mode = mcp2515_do_set_mode;
	priv->can.ctrlmode_supported = CAN_CTRLMODE_3_SAMPLES |
		CAN_CTRLMODE_LOOPBACK | CAN_CTRLMODE_LISTENONLY;
	priv->net = net;

	spi_set_drvdata(spi, priv);
	priv->spi = spi;
	mutex_init(&priv->lock);

	priv->spi_tx_buf = devm_kzalloc(&spi->dev, SPI_TRANSFER_LEN,
					GFP_KERNEL);
	if (!priv->spi_tx_buf)
		return -ENOMEM;

	priv->spi_rx_buf = devm_kzalloc(&spi->dev, SPI_TRANSFER_LEN,
					GFP_KERNEL);
	if (!priv->spi_rx_buf)
		return -ENOMEM;

	SET_NETDEV_DEV(net, &spi->dev);

	ret = register_candev(net);
	if (ret) {
		pr_err("can not register can device\n");
		free_candev(net);
		return ret;
	}

	devm_can_led_init(net);

	pr_info("Tung: register Can network device OK\n");

	return 0;
}

static int dev_remove(struct spi_device *spi)
{
	struct mcp2515_can_priv *priv = spi_get_drvdata(spi);
	struct net_device *net = priv->net;

	unregister_candev(net);
	free_candev(net);

	return 0;
}

static struct of_device_id can_of_match[] = {
	{ .compatible = "mcp,mcp2515" },
	{ /* NULL */ },
};
MODULE_DEVICE_TABLE(of, can_of_match);

static struct spi_driver can_driver = {
	.driver = {
		.name = "mcp2515",
		.of_match_table = can_of_match,
		.owner = THIS_MODULE,
	},
	.probe = dev_probe,
	.remove = dev_remove,
};

module_spi_driver(can_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tungnt<Thanhtungdt4.haui@gmail.com>");
MODULE_DESCRIPTION("can network driver\n");
