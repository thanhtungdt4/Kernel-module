#ifndef _MCP2515_
#define _MCP2515_

#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/types.h>
#include <linux/jiffies.h>

typedef unsigned char		uint8_t;

#define TIMEOUTVALUE		50
#define MCP_SIDH		0
#define MCP_SIDL		1
#define MCP_EID8		2
#define MCP_EID0		3

#define MCP_TXB_EXIDE_M		0x08
#define MCP_DLC_MASK		0x0F
#define MCP_RTR_MASK		0x40

#define MCP_RXB_RX_ANY		0x60
#define MCP_RXB_RX_EXT		0x40
#define MCP_RXB_RX_STD		0x20
#define MCP_RXB_RX_STDEXT	0x00
#define MCP_RXB_RX_MASK		0x60
#define MCP_RXB_BUKT_MASK	(1 << 2)

#define MCP_TXB_TXBUFE_M	0x80
#define MCP_TXB_ABTF_M		0x40
#define MCP_TXB_MLOA_M		0x20
#define MCP_TXB_TXERR_M		0x10
#define MCP_TXB_TXREQ_M		0x08
#define MCP_TXB_TXIE_M		0x04
#define MCP_TXB_TXP10_M		0x03

#define MCP_TXB_RTR_M		0x40
#define MCP_RXB_IDE_M		0x08
#define MCP_RXB_RTR_M		0x40

#define MCP_STAT_TX_PENDING_MASK	0x54
#define MCP_STAT_TX0_PENDING		0x04
#define MCP_STAT_TX1_PENDING		0x10
#define MCP_STAT_TX2_PENDING		0x40
#define MCP_STAT_TXIF_MASK		0xA8
#define MCP_STAT_TX0IF			0x08
#define MCP_STAT_TX1IF			0x20
#define MCP_STAT_TX2IF			0x80
#define MCP_STAT_RXIF_MASK		0x03
#define MCP_STAT_RX0IF			(1 << 0)
#define MCP_STAT_RX1IF			(1 << 1)

#define MCP_EFLG_RX1OVR		(1 << 7)
#define MCP_EFLG_RX0OVR		(1 << 6)
#define MCP_EFLG_TXBO		(1 << 5)
#define MCP_EFLG_TXEP		(1 << 4)
#define MCP_EFLG_RXEP		(1 << 3)
#define MCP_EFLG_TXWAR		(1 << 2)
#define MCP_EFLG_RXWAR		(1 << 1)
#define MCP_EFLG_EWARN		(1 << 0)
#define MCP_EFLG_ERRORMASK	0xF8

#define MCP_RXF0SIDH		0x00
#define MCP_RXF0SIDL		0x01
#define MCP_RXF0EID8		0x02
#define MCP_RXF0EID0		0x03
#define MCP_RXF1SIDH		0x04
#define MCP_RXF1SIDL		0x05
#define MCP_RXF1EID8		0x06
#define MCP_RXF1EID0		0x07
#define MCP_RXF2SIDH		0x08
#define MCP_RXF2SIDL		0x09
#define MCP_RXF2EID8		0x0A
#define MCP_RXF2EID0		0x0B
#define MCP_BFPCTRL		0x0C
#define MCP_TXRTSCTRL		0x0D
#define MCP_CANSTAT		0x0E
#define MCP_CANCTRL		0x0F
#define MCP_RXF3SIDH		0x10
#define MCP_RXF3SIDL		0x11
#define MCP_RXF3EID8		0x12
#define MCP_RXF3EID0		0x13
#define MCP_RXF4SIDH		0x14
#define MCP_RXF4SIDL		0x15
#define MCP_RXF4EID8		0x16
#define MCP_RXF4EID0		0x17
#define MCP_RXF5SIDH		0x18
#define MCP_RXF5SIDL		0x19
#define MCP_RXF5EID8		0x1A
#define MCP_RXF5EID0		0x1B
#define MCP_TEC			0x1C
#define MCP_REC			0x1D
#define MCP_RXM0SIDH		0x20
#define MCP_RXM0SIDL		0x21
#define MCP_RXM0EID8		0x22
#define MCP_RXM0EID0		0x23
#define MCP_RXM1SIDH		0x24
#define MCP_RXM1SIDL		0x25
#define MCP_RXM1EID8		0x26
#define MCP_RXM1EID0		0x27
#define MCP_CNF3		0x28
#define MCP_CNF2		0x29
#define MCP_CNF1		0x2A
#define MCP_CANINTE		0x2B
#define MCP_CANINTF		0x2C
#define MCP_EFLG		0x2D
#define MCP_TXB0CTRL		0x30
#define MCP_TXB0SIDH		0x31
#define MCP_TXB1CTRL		0x40
#define MCP_TXB1SIDH		0x41
#define MCP_TXB2CTRL		0x50
#define MCP_TXB2SIDH		0x51
#define MCP_RXB0CTRL		0x60
#define MCP_RXB0SIDH		0x61
#define MCP_RXB1CTRL		0x70
#define MCP_RXB1SIDH		0x71

#define MCP_TX_INT		0x1C
#define MCP_TX01_INT		0x0C
#define MCP_RX_INT		0x03
#define MCP_NO_INT		0x00

#define MCP_TX01_MASK		0x14
#define MCP_TX_MASK		0x54

#define MCP_WRITE		0x02
#define MCP_READ		0x03
#define MCP_BITMOD		0x05
#define MCP_LOAD_TX0		0x40
#define MCP_LOAD_TX1		0x42
#define MCP_LOAD_TX2		0x44

#define MCP_RTS_TX0		0x81
#define MCP_RTS_TX1		0x82
#define MCP_RTS_TX2		0x84
#define MCP_RTS_ALL		0x87
#define MCP_READ_RX0		0x90
#define MCP_READ_RX1		0x94
#define MCP_READ_STATUS		0xA0
#define MCP_RX_STATUS		0xB0
#define MCP_RESET		0xC0

#define MODE_NORMAL		0x00
#define MODE_SLEEP		0x20
#define MODE_LOOPBACK		0x40
#define MODE_LISTENONLY		0x60
#define MODE_CONFIG		0x80
#define MODE_POWERUP		0xE0
#define MODE_MASK		0xE0
#define ABORT_TX		0x10
#define MODE_ONESHOT		0x08
#define CLKOUT_ENABLE		0x04
#define CLKOUT_DISABLE		0x00
#define CLKOUT_PS1		0x00
#define CLKOUT_PS2		0x01
#define CLKOUT_PS4		0x02
#define CLKOUT_PS8		0x03

#define SJW1			0x00
#define SJW2			0x40
#define SJW3			0x80
#define SJW4			0xC0

#define BTLMODE			0x80
#define SAMPLE_1X		0x00
#define SAMPLE_3X		0x40

#define SOF_ENABLE		0x80
#define SOF_DISABLE		0x00
#define WAKFIL_ENABLE		0x40
#define WAKFIL_DISABLE		0x00

#define MCP_RX0IF		0x01
#define MCP_RX1IF		0x02
#define MCP_TX0IF		0x04
#define MCP_TX1IF		0x08
#define MCP_TX2IF		0x10
#define MCP_ERRIF		0x20
#define MCP_WAKIF		0x40
#define MCP_MERRF		0x80

#define B1BFS			0x20
#define B0BFS			0x10
#define B1BFE			0x08
#define B0BFE			0x04
#define B1BFM			0x02
#define B0BFM			0x01

#define B2RTS			0x20
#define B1RTS			0x10
#define B0RTS			0x08
#define B2RTSM			0x04
#define B1RTSM			0x02
#define B0RTSM			0x01

#define MCP_16MHz		1
#define MCP_8MHz		2

#define MCP_16MHz_1000kBPS_CFG1		0x00
#define MCP_16MHz_1000kBPS_CFG2		0xD0
#define MCP_16MHz_1000kBPS_CFG3		0x82

#define MCP_16MHz_500kBPS_CFG1		0x00
#define MCP_16MHz_500kBPS_CFG2		0xF0
#define MCP_16MHz_500kBPS_CFG3		0x86

#define MCP_16MHz_250kBPS_CFG1		0x41
#define MCP_16MHz_250kBPS_CFG2		0xF1
#define MCP_16MHz_250kBPS_CFG3		0x85

#define MCP_16MHz_200kBPS_CFG1		0x01
#define MCP_16MHz_200kBPS_CFG2		0xFA
#define MCP_16MHz_200kBPS_CFG3		0x87

#define MCP_16MHz_125kBPS_CFG1		0x03
#define MCP_16MHz_125kBPS_CFG2		0xF0
#define MCP_16MHz_125kBPS_CFG3		0x86

#define MCP_16MHz_100kBPS_CFG1		0x03
#define MCP_16MHz_100kBPS_CFG2		0xFA
#define MCP_16MHz_100kBPS_CFG3		0x87

#define MCP_16MHz_95kBPS_CFG1		0x03
#define MCP_16MHz_95kBPS_CFG2		0xAD
#define MCP_16MHz_95kBPS_CFG3		0x07

#define MCP_16MHz_83k3BPS_CFG1		0x03
#define MCP_16MHz_83k3BPS_CFG2		0xBE
#define MCP_16MHz_83k3BPS_CFG3		0x07

#define MCP_16MHz_80kBPS_CFG1		0x03
#define MCP_16MHz_80kBPS_CFG2		0xFF
#define MCP_16MHz_80kBPS_CFG3		0x87

#define MCP_16MHz_50kBPS_CFG1		0x07
#define MCP_16MHz_50kBPS_CFG2		0xFA
#define MCP_16MHz_50kBPS_CFG3		0x87

#define MCP_16MHz_40kBPS_CFG1		0x07
#define MCP_16MHz_40kBPS_CFG2		0xFF
#define MCP_16MHz_40kBPS_CFG3		0x87

#define MCP_16MHz_33kBPS_CFG1		0x09
#define MCP_16MHz_33kBPS_CFG2		0xBE
#define MCP_16MHz_33kBPS_CFG3		0x07

#define MCP_16MHz_31k25BPS_CFG1		0x0F
#define MCP_16MHz_31k25BPS_CFG2		0xF1
#define MCP_16MHz_31k25BPS_CFG3		0x85

#define MCP_16MHz_25kBPS_CFG1		0x0F
#define MCP_16MHz_25kBPS_CFG2		0xBA
#define MCP_16MHz_25kBPS_CFG3		0x07

#define MCP_16MHz_20kBPS_CFG1		0x0F
#define MCP_16MHz_20kBPS_CFG2		0xFF
#define MCP_16MHz_20kBPS_CFG3		0x87

#define MCP_16MHz_10kBPS_CFG1		0x1F
#define MCP_16MHz_10kBPS_CFG2		0xFF
#define MCP_16MHz_10kBPS_CFG3		0x87

#define MCP_16MHz_5kBPS_CFG1		0x3F
#define MCP_16MHz_5kBPS_CFG2		0xFF
#define MCP_16MHz_5kBPS_CFG3		0x87

#define MCP_16MHz_666kBPS_CFG1		0x00
#define MCP_16MHz_666kBPS_CFG2		0xA0
#define MCP_16MHz_666kBPS_CFG3		0x04

#define MCP_8MHz_1000kBPS_CFG1		0x00
#define MCP_8MHz_1000kBPS_CFG2		0x80
#define MCP_8MHz_1000kBPS_CFG3		0x00

#define MCP_8MHz_500kBPS_CFG1		0x00
#define MCP_8MHz_500kBPS_CFG2		0x90
#define MCP_8MHz_500kBPS_CFG3		0x02

#define MCP_8MHz_250kBPS_CFG1		0x00
#define MCP_8MHz_250kBPS_CFG2		0xB1
#define MCP_8MHz_250kBPS_CFG3		0x05

#define MCP_8MHz_200kBPS_CFG1		0x00
#define MCP_8MHz_200kBPS_CFG2		0xB4
#define MCP_8MHz_200kBPS_CFG3		0x06

#define MCP_8MHz_125kBPS_CFG1		0x01
#define MCP_8MHz_125kBPS_CFG2		0xB1
#define MCP_8MHz_125kBPS_CFG3		0x05

#define MCP_8MHz_100kBPS_CFG1		0x01
#define MCP_8MHz_100kBPS_CFG2		0xB4
#define MCP_8MHz_100kBPS_CFG3		0x06

#define MCP_8MHz_80kBPS_CFG1		0x01
#define MCP_8MHz_80kBPS_CFG2		0xBF
#define MCP_8MHz_80kBPS_CFG3		0x07

#define MCP_8MHz_50kBPS_CFG1		0x03
#define MCP_8MHz_50kBPS_CFG2		0xB4
#define MCP_8MHz_50kBPS_CFG3		0x06

#define MCP_8MHz_40kBPS_CFG1		0x03
#define MCP_8MHz_40kBPS_CFG2		0xBF
#define MCP_8MHz_40kBPS_CFG3		0x07

#define MCP_8MHz_31k25BPS_CFG1		0x07
#define MCP_8MHz_31k25BPS_CFG2		0xA4
#define MCP_8MHz_31k25BPS_CFG3		0x04

#define MCP_8MHz_20kBPS_CFG1		0x07
#define MCP_8MHz_20kBPS_CFG2		0xBF
#define MCP_8MHz_20kBPS_CFG3		0x07

#define MCP_8MHz_10kBPS_CFG1		0x0F
#define MCP_8MHz_10kBPS_CFG2		0xBF
#define MCP_8MHz_10kBPS_CFG3		0x07

#define MCP_8MHz_5kBPS_CFG1		0x1F
#define MCP_8MHz_5kBPS_CFG2		0xBF
#define MCP_8MHz_5kBPS_CFG3		0x07

#define MCPDEBUG			0
#define MCPDEBUG_TXBUF			0
#define MCP_N_TXBUFFERS			3

#define MCP_RXBUF_0		MCP_RXB0SIDH
#define MCP_RXBUF_1		MCP_RXB1SIDH

#define MCP2515_OK			0
#define MCP2515_FAIL			1
#define MCP_ALLTXBUSY			2

#define CANDEBUG			1
#define CANUSELOOP			0

#define CANSENDTIMEOUT			(200)

#define MCP_PIN_HIZ		0
#define MCP_PIN_INT		1
#define MCP_PIN_OUT		2
#define MCP_PIN_IN		3

#define MCP_RX0BF		0
#define MCP_RX1BF		1
#define MCP_TX0RTS		2
#define MCP_TX1RTS		3
#define MCP_TX2RTS		4

#define CANAUTOPROCESS		1
#define CANAUTOON		1
#define CANAUTOOFF		0
#define CAN_STDID		0
#define CAN_EXTID		1
#define CANDEFAULTIDENT		(0x55CC)
#define CANDEFAULTIDENTEXT	(CAN_EXTID)

#define CAN_5KBPS		1
#define CAN_10KBPS		2
#define CAN_20KBPS		3
#define CAN_25KBPS		4
#define CAN_31K25BPS		5
#define CAN_33KBPS		6
#define CAN_40KBPS		7
#define CAN_50KBPS		8
#define CAN_80KBPS		9
#define CAN_83K3BPS		10
#define CAN_95KBPS		11
#define CAN_100KBPS		12
#define CAN_125KBPS		13
#define CAN_200KBPS		14
#define CAN_250KBPS		15
#define CAN_500KBPS		16
#define CAN_666KBPS		17
#define CAN_1000KBPS		18

#define CAN_OK			(0)
#define CAN_FAILINIT		(1)
#define CAN_FAILTX		(2)
#define CAN_MSGAVAIL		(3)
#define CAN_NOMSG		(4)
#define CAN_CTRLERROR		(5)
#define CAN_GETTXBFTIMEOUT	(6)
#define CAN_SENDMSGTIMEOUT	(7)
#define CAN_FAIL		(0xFF)

#define CAN_MAX_CHAR_IN_MESSAGE	(8)

void gpio_set_pin(int gpio, int state);
void SPI_Tx(void *spi_dev, uint8_t data);
uint8_t SPI_Rx(void *spi_dev);
uint8_t txCtrlReg(uint8_t i);
uint8_t statusToTxBuffer(uint8_t status);
uint8_t statusToTxSidh(uint8_t status);
uint8_t txSidhToRTS(uint8_t sidh);
uint8_t txSidhToTxLoad(uint8_t sidh);
uint8_t txIfFlag(uint8_t i);
uint8_t txStatusPendingFlag(uint8_t i);
void mcp2515_reset(void *spi_dev);
uint8_t mcp2515_readRegister(void *spi_dev, uint8_t address);
void mcp2515_readRegisterS(void *spi_dev, uint8_t address, uint8_t values[],
				uint8_t n);
void mcp2515_setRegister(void *spi_dev, uint8_t address, uint8_t value);
void mcp2515_setRegisterS(void *spi_dev, uint8_t address, uint8_t values[],
				uint8_t n);
void mcp2515_modifyRegister(void *spi_dev, uint8_t address, uint8_t mask,
				uint8_t data);
uint8_t mcp2515_readStatus(void *spi_dev);
void setSleepWakeup(void *spi_dev, uint8_t enable);
uint8_t getMode(void *spi_dev);
uint8_t mcp2515_requestNewMode(void *spi_dev, uint8_t newmode);
uint8_t mcp2515_setCANCTRL_Mode(void *spi_dev, uint8_t newmode);
uint8_t sleep(void *spi_dev);
uint8_t wake(void *spi_dev);
uint8_t setMode(void *spi_dev, uint8_t opMode);
uint8_t mcp2515_configRate(void *spi_dev, uint8_t canSpeed, uint8_t clock);
void mcp2515_initCANBuffers(void *spi_dev);
uint8_t mcp2515_init(void *spi_dev, uint8_t canSpeed, uint8_t clock);
void mcp2515_id_to_buf(uint8_t ext, unsigned long id,
			uint8_t *tbufdata);
void mcp2515_write_id(void *spi_dev, uint8_t mcp_addr, uint8_t ext,
			unsigned long id);
void mcp2515_read_id(void *spi_dev, uint8_t mcp_addr, uint8_t *ext,
			unsigned long *id);
void mcp2515_start_transmit(void *spi_dev, uint8_t mcp_addr);
void mcp2515_write_canMsg(void *spi_dev, uint8_t buffer_sidh_addr,
			unsigned long id, uint8_t ext, uint8_t rtrBit,
			uint8_t len, uint8_t *buf);
void mcp2515_read_canMsg(void *spi_dev, uint8_t buffer_load_addr,
			unsigned long *id, uint8_t *ext, uint8_t *rtrBit,
			uint8_t *len, uint8_t *buf);
uint8_t mcp2515_isTXBufFree(void *spi_dev, uint8_t *txbuf_n, uint8_t iBuf);
uint8_t mcp2515_getNextFreeTXBuf(void *spi_dev, uint8_t *txbuf_n);
uint8_t can_begin(void *spi_dev, uint8_t speedset, uint8_t clockset);
void enableTxInterrupt(void *spi_dev, bool enale);
uint8_t init_Mask(void *spi_dev, uint8_t num, uint8_t ext, unsigned long ulData);
uint8_t init_Filt(void *spi_dev, uint8_t num, uint8_t ext, unsigned long ulData);
uint8_t sendMsgBuf1(void *spi_dev, uint8_t status, unsigned long id, uint8_t ext,
			uint8_t rtrBit, uint8_t len, uint8_t *buf);
uint8_t trySendMsgBuf(void *spi_dev, unsigned long id, uint8_t ext,
			uint8_t rtrBit, uint8_t len, uint8_t *buf,
			uint8_t iTxBuf);
uint8_t sendMsg(void *spi_dev, unsigned long id, uint8_t ext, uint8_t rtrBit,
		uint8_t len, uint8_t *buf, bool wait_sent);
uint8_t sendMsgBuf(void *spi_dev, unsigned long id, uint8_t ext, uint8_t rtrBit,
			uint8_t len, uint8_t *buf, bool wait_sent);

uint8_t readMsgBufID(void *spi_dev, uint8_t status, unsigned long *id,
		uint8_t *ext, uint8_t *rtrBit, uint8_t *len, uint8_t *buf);

uint8_t readMsgBuf(void *spi_dev, uint8_t *len, uint8_t buf[]);
uint8_t readRxTxStatus(void *spi_dev);
uint8_t checkClearRxStatus(uint8_t *status);
uint8_t checkClearTxStatus(uint8_t *status, uint8_t iTxBuf);
void clearBufferTransmitIfFlags(void *spi_dev, uint8_t flags);
uint8_t checkReceive(void *spi_dev);
uint8_t checkError(void *spi_dev);
unsigned long getCanId(void);
uint8_t isRemoteRequest(void);
uint8_t isExtendedFrame(void);
bool mcpDigitalWrite(void *spi_dev, uint8_t pin, uint8_t mode);

#endif

