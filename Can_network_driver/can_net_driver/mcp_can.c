#include "mcp_can_dfs.h"

#define CS_PIN		24

uint8_t ext_flg;
uint8_t rtr;
uint8_t SPICS;
uint8_t nReservedTx;
uint8_t mcpMode;
unsigned long can_id;

void gpio_set_pin(int gpio, int state)
{
	gpio_direction_output(gpio, 1);
	gpio_set_value(gpio, state);
}

#define MCP2515_SELECT()	gpio_set_pin(CS_PIN, 0)
#define MCP2515_UNSELECT()	gpio_set_pin(CS_PIN, 1)

void SPI_Tx(void *spi_dev, uint8_t data)
{
	spi_write(spi_dev, &data, 1);
}

uint8_t SPI_Rx(void *spi_dev)
{
	uint8_t ret;

	spi_read(spi_dev, &ret, 1);

	return ret;
}

uint8_t txCtrlReg(uint8_t i)
{
	switch (i) {
	case 0: return MCP_TXB0CTRL;
	case 1: return MCP_TXB1CTRL;
	case 2: return MCP_TXB2CTRL;
	}

	return MCP_TXB2CTRL;
}

uint8_t statusToTxBuffer(uint8_t status)
{
	switch (status) {
	case MCP_TX0IF: return 0;
	case MCP_TX1IF: return 1;
	case MCP_TX2IF: return 2;
	}

	return 0xFF;
}

uint8_t statusToTxSidh(uint8_t status)
{
	switch (status) {
	case MCP_TX0IF: return MCP_TXB0SIDH;
	case MCP_TX1IF: return MCP_TXB1SIDH;
	case MCP_TX2IF: return MCP_TXB2SIDH;
	}

	return 0;
}

uint8_t txSidhToRTS(uint8_t sidh)
{
	switch (sidh) {
	case MCP_TXB0SIDH: return MCP_RTS_TX0;
	case MCP_TXB1SIDH: return MCP_RTS_TX1;
	case MCP_TXB2SIDH: return MCP_RTS_TX2;
	}

	return 0;
}

uint8_t txSidhToTxLoad(uint8_t sidh)
{
	switch (sidh) {
	case MCP_TXB0SIDH: return MCP_LOAD_TX0;
	case MCP_TXB1SIDH: return MCP_LOAD_TX1;
	case MCP_TXB2SIDH: return MCP_LOAD_TX2;
	}

	return 0;
}

uint8_t txIfFlag(uint8_t i)
{
	switch (i) {
	case 0: return MCP_TX0IF;
	case 1: return MCP_TX1IF;
	case 2: return MCP_TX2IF;
	}

	return 0;
}

uint8_t txStatusPendingFlag(uint8_t i)
{
	switch (i) {
	case 0: return MCP_STAT_TX0_PENDING;
	case 1: return MCP_STAT_TX1_PENDING;
	case 2: return MCP_STAT_TX2_PENDING;
	}

	return 0xFF;
}

void mcp2515_reset(void *spi_dev)
{
	MCP2515_SELECT();
	SPI_Tx(spi_dev, MCP_RESET);
	MCP2515_UNSELECT();
	mdelay(10);
}

uint8_t mcp2515_readRegister(void *spi_dev, uint8_t address)
{
	uint8_t ret;

	MCP2515_SELECT();
	SPI_Tx(spi_dev, MCP_READ);
	SPI_Tx(spi_dev, address);
	ret = SPI_Rx(spi_dev);

	return ret;
}

void mcp2515_readRegisterS(void *spi_dev, uint8_t address, uint8_t values[],
				uint8_t n)
{
	uint8_t i;

	MCP2515_SELECT();
	SPI_Tx(spi_dev, MCP_READ);
	SPI_Tx(spi_dev, address);

	for (i = 0; i < n && i < CAN_MAX_CHAR_IN_MESSAGE; i++) {
		values[i] = SPI_Rx(spi_dev);
	}

	MCP2515_UNSELECT();
}

void mcp2515_setRegister(void *spi_dev, uint8_t address, uint8_t value)
{
	MCP2515_SELECT();
	SPI_Tx(spi_dev, MCP_WRITE);
	SPI_Tx(spi_dev, address);
	SPI_Tx(spi_dev, value);
	MCP2515_UNSELECT();
}

void mcp2515_setRegisterS(void *spi_dev, uint8_t address, uint8_t values[],
				uint8_t n)
{
	uint8_t i;

	MCP2515_SELECT();
	SPI_Tx(spi_dev, MCP_WRITE);
	SPI_Tx(spi_dev, address);

	for (i = 0; i < n; i++) {
		SPI_Tx(spi_dev, values[i]);
	}

	MCP2515_UNSELECT();
}

void mcp2515_modifyRegister(void *spi_dev, uint8_t address, uint8_t mask,
				uint8_t data)
{
	MCP2515_SELECT();
	SPI_Tx(spi_dev, MCP_BITMOD);
	SPI_Tx(spi_dev, address);
	SPI_Tx(spi_dev, mask);
	SPI_Tx(spi_dev, data);
	MCP2515_UNSELECT();
}

uint8_t mcp2515_readStatus(void *spi_dev)
{
	uint8_t i;

	MCP2515_SELECT();
	SPI_Tx(spi_dev, MCP_READ_STATUS);
	i = SPI_Rx(spi_dev);
	MCP2515_UNSELECT();

	return i;
}

void setSleepWakeup(void *spi_dev, uint8_t enable)
{
	mcp2515_modifyRegister(spi_dev, MCP_CANINTE, MCP_WAKIF,
				enable ? MCP_WAKIF : 0);
}

uint8_t getMode(void *spi_dev)
{
	return mcp2515_readRegister(spi_dev, MCP_CANSTAT) & MODE_MASK;
}

uint8_t mcp2515_requestNewMode(void *spi_dev, uint8_t newmode)
{
	unsigned long startTime = jiffies;
	uint8_t statReg;

	while (1) {
		mcp2515_modifyRegister(spi_dev, MCP_CANCTRL, MODE_MASK, newmode);
		statReg = mcp2515_readRegister(spi_dev, MCP_CANSTAT);

		if ((statReg & MODE_MASK) == newmode)
			return MCP2515_OK;
		else if ((jiffies - startTime) > 500)
			return MCP2515_FAIL;
	}
}

uint8_t mcp2515_setCANCTRL_Mode(void *spi_dev, uint8_t newmode)
{
	uint8_t wakeIntEnabled;

	if ((getMode(spi_dev)) == MODE_SLEEP && newmode != MODE_SLEEP) {
		wakeIntEnabled = (mcp2515_readRegister(spi_dev, MCP_CANINTE)
					& MCP_WAKIF);
		if (!wakeIntEnabled)
			mcp2515_modifyRegister(spi_dev, MCP_CANINTE, MCP_WAKIF,
						MCP_WAKIF);
		mcp2515_modifyRegister(spi_dev, MCP_CANINTF, MCP_WAKIF,
					MCP_WAKIF);

		if (mcp2515_requestNewMode(spi_dev, MODE_LISTENONLY) != MCP2515_OK)
			return MCP2515_FAIL;
		if (!wakeIntEnabled)
			mcp2515_modifyRegister(spi_dev, MCP_CANINTE, MCP_WAKIF, 0);
	}

	mcp2515_modifyRegister(spi_dev, MCP_CANINTF, MCP_WAKIF, 0);

	return mcp2515_requestNewMode(spi_dev, newmode);
}

uint8_t sleep(void *spi_dev)
{
	if (getMode(spi_dev) != MODE_SLEEP)
		return mcp2515_setCANCTRL_Mode(spi_dev, MODE_SLEEP);
	else
		return CAN_OK;
}

uint8_t wake(void *spi_dev)
{
	uint8_t currMode = getMode(spi_dev);

	if (currMode != mcpMode)
		return mcp2515_setCANCTRL_Mode(spi_dev, mcpMode);
	else
		return CAN_OK;
}

uint8_t setMode(void *spi_dev, uint8_t opMode)
{
	if (opMode != MODE_SLEEP)
		mcpMode = opMode;
	return mcp2515_setCANCTRL_Mode(spi_dev, opMode);
}

uint8_t mcp2515_configRate(void *spi_dev, uint8_t canSpeed, uint8_t clock)
{
	uint8_t set = 1;
	uint8_t cfg1, cfg2, cfg3;

	switch (clock) {
	case MCP_16MHz:
		switch (canSpeed) {
		case CAN_5KBPS:
			cfg1 = MCP_16MHz_5kBPS_CFG1;
			cfg2 = MCP_16MHz_5kBPS_CFG2;
			cfg3 = MCP_16MHz_5kBPS_CFG3;
			break;

		case CAN_10KBPS:
			cfg1 = MCP_16MHz_10kBPS_CFG1;
			cfg2 = MCP_16MHz_10kBPS_CFG2;
			cfg3 = MCP_16MHz_10kBPS_CFG3;
			break;

		case CAN_20KBPS:
			cfg1 = MCP_16MHz_20kBPS_CFG1;
			cfg2 = MCP_16MHz_20kBPS_CFG2;
			cfg3 = MCP_16MHz_20kBPS_CFG3;
			break;

		case CAN_25KBPS:
			cfg1 = MCP_16MHz_25kBPS_CFG1;
			cfg2 = MCP_16MHz_25kBPS_CFG2;
			cfg3 = MCP_16MHz_25kBPS_CFG3;
			break;

		case CAN_31K25BPS:
			cfg1 = MCP_16MHz_31k25BPS_CFG1;
			cfg2 = MCP_16MHz_31k25BPS_CFG2;
			cfg3 = MCP_16MHz_31k25BPS_CFG3;
			break;

		case CAN_33KBPS:
			cfg1 = MCP_16MHz_33kBPS_CFG1;
			cfg2 = MCP_16MHz_33kBPS_CFG2;
			cfg3 = MCP_16MHz_33kBPS_CFG3;
			break;

		case CAN_40KBPS:
			cfg1 = MCP_16MHz_40kBPS_CFG1;
			cfg2 = MCP_16MHz_40kBPS_CFG2;
			cfg3 = MCP_16MHz_40kBPS_CFG3;
			break;

		case CAN_50KBPS:
			cfg1 = MCP_16MHz_50kBPS_CFG1;
			cfg2 = MCP_16MHz_50kBPS_CFG2;
			cfg3 = MCP_16MHz_50kBPS_CFG3;
			break;

		case CAN_80KBPS:
			cfg1 = MCP_16MHz_80kBPS_CFG1;
			cfg2 = MCP_16MHz_80kBPS_CFG2;
			cfg3 = MCP_16MHz_80kBPS_CFG3;
			break;

		case CAN_83K3BPS:
			cfg1 = MCP_16MHz_83k3BPS_CFG1;
			cfg2 = MCP_16MHz_83k3BPS_CFG2;
			cfg3 = MCP_16MHz_83k3BPS_CFG3;
			break;

		case CAN_95KBPS:
			cfg1 = MCP_16MHz_95kBPS_CFG1;
			cfg2 = MCP_16MHz_95kBPS_CFG2;
			cfg3 = MCP_16MHz_95kBPS_CFG3;
			break;

		case CAN_100KBPS:
			cfg1 = MCP_16MHz_100kBPS_CFG1;
			cfg2 = MCP_16MHz_100kBPS_CFG2;
			cfg3 = MCP_16MHz_100kBPS_CFG3;
			break;

		case CAN_125KBPS:
			cfg1 = MCP_16MHz_125kBPS_CFG1;
			cfg2 = MCP_16MHz_125kBPS_CFG2;
			cfg3 = MCP_16MHz_125kBPS_CFG3;
			break;

		case CAN_200KBPS:
			cfg1 = MCP_16MHz_200kBPS_CFG1;
			cfg2 = MCP_16MHz_200kBPS_CFG2;
			cfg3 = MCP_16MHz_200kBPS_CFG3;
			break;

		case CAN_250KBPS:
			cfg1 = MCP_16MHz_250kBPS_CFG1;
			cfg2 = MCP_16MHz_250kBPS_CFG2;
			cfg3 = MCP_16MHz_250kBPS_CFG3;
			break;

		case CAN_500KBPS:
			cfg1 = MCP_16MHz_500kBPS_CFG1;
			cfg2 = MCP_16MHz_500kBPS_CFG2;
			cfg3 = MCP_16MHz_500kBPS_CFG3;
			break;

		case CAN_666KBPS:
			cfg1 = MCP_16MHz_666kBPS_CFG1;
			cfg2 = MCP_16MHz_666kBPS_CFG2;
			cfg3 = MCP_16MHz_666kBPS_CFG3;
			break;

		case CAN_1000KBPS:
			cfg1 = MCP_16MHz_1000kBPS_CFG1;
			cfg2 = MCP_16MHz_1000kBPS_CFG2;
			cfg3 = MCP_16MHz_1000kBPS_CFG3;
			break;

		default:
			set = 0;
			break;
		}
		break;

	case MCP_8MHz:
		switch (canSpeed) {
		case CAN_5KBPS:
			cfg1 = MCP_8MHz_5kBPS_CFG1;
			cfg2 = MCP_8MHz_5kBPS_CFG2;
			cfg3 = MCP_8MHz_5kBPS_CFG3;
			break;

		case CAN_10KBPS:
			cfg1 = MCP_8MHz_10kBPS_CFG1;
			cfg2 = MCP_8MHz_10kBPS_CFG2;
			cfg3 = MCP_8MHz_10kBPS_CFG3;
			break;

		case CAN_20KBPS:
			cfg1 = MCP_8MHz_20kBPS_CFG1;
			cfg2 = MCP_8MHz_20kBPS_CFG2;
			cfg3 = MCP_8MHz_20kBPS_CFG3;
			break;

		case CAN_31K25BPS:
			cfg1 = MCP_8MHz_31k25BPS_CFG1;
			cfg2 = MCP_8MHz_31k25BPS_CFG2;
			cfg3 = MCP_8MHz_31k25BPS_CFG3;
			break;

		case CAN_40KBPS:
			cfg1 = MCP_8MHz_40kBPS_CFG1;
			cfg2 = MCP_8MHz_40kBPS_CFG2;
			cfg3 = MCP_8MHz_40kBPS_CFG3;
			break;

		case CAN_50KBPS:
			cfg1 = MCP_8MHz_50kBPS_CFG1;
			cfg2 = MCP_8MHz_50kBPS_CFG2;
			cfg3 = MCP_8MHz_50kBPS_CFG3;
			break;

		case CAN_80KBPS:
			cfg1 = MCP_8MHz_80kBPS_CFG1;
			cfg2 = MCP_8MHz_80kBPS_CFG2;
			cfg3 = MCP_8MHz_80kBPS_CFG3;
			break;

		case CAN_100KBPS:
			cfg1 = MCP_8MHz_100kBPS_CFG1;
			cfg2 = MCP_8MHz_100kBPS_CFG2;
			cfg3 = MCP_8MHz_100kBPS_CFG3;
			break;

		case CAN_125KBPS:
			cfg1 = MCP_8MHz_125kBPS_CFG1;
			cfg2 = MCP_8MHz_125kBPS_CFG2;
			cfg3 = MCP_8MHz_125kBPS_CFG3;
			break;

		case CAN_200KBPS:
			cfg1 = MCP_8MHz_200kBPS_CFG1;
			cfg2 = MCP_8MHz_200kBPS_CFG2;
			cfg3 = MCP_8MHz_200kBPS_CFG3;
			break;

		case CAN_250KBPS:
			cfg1 = MCP_8MHz_250kBPS_CFG1;
			cfg2 = MCP_8MHz_250kBPS_CFG2;
			cfg3 = MCP_8MHz_250kBPS_CFG3;
			break;

		case CAN_500KBPS:
			cfg1 = MCP_8MHz_500kBPS_CFG1;
			cfg2 = MCP_8MHz_500kBPS_CFG2;
			cfg3 = MCP_8MHz_500kBPS_CFG3;
			break;

		case CAN_1000KBPS:
			cfg1 = MCP_8MHz_1000kBPS_CFG1;
			cfg2 = MCP_8MHz_1000kBPS_CFG2;
			cfg3 = MCP_8MHz_1000kBPS_CFG3;
			break;

		default:
			set = 0;
			break;
		}
		break;
	default:
		set = 0;
		break;
	}

	if (set) {
		mcp2515_setRegister(spi_dev, MCP_CNF1, cfg1);
		mcp2515_setRegister(spi_dev, MCP_CNF2, cfg2);
		mcp2515_setRegister(spi_dev, MCP_CNF3, cfg3);
		return MCP2515_OK;
	} else
		return MCP2515_FAIL;
}

void mcp2515_initCANBuffers(void *spi_dev)
{
	uint8_t i, a1, a2, a3;

	a1 = MCP_TXB0CTRL;
	a2 = MCP_TXB1CTRL;
	a3 = MCP_TXB2CTRL;

	for (i = 0; i < 14; i++) {
		mcp2515_setRegister(spi_dev, a1, 0);
		mcp2515_setRegister(spi_dev, a2, 0);
		mcp2515_setRegister(spi_dev, a3, 0);

		a1++;
		a2++;
		a3++;
	}

	mcp2515_setRegister(spi_dev, MCP_RXB0CTRL, 0);
	mcp2515_setRegister(spi_dev, MCP_RXB1CTRL, 0);
}

uint8_t mcp2515_init(void *spi_dev, uint8_t canSpeed, uint8_t clock)
{
	uint8_t res;

	mcp2515_reset(spi_dev);

	res = mcp2515_setCANCTRL_Mode(spi_dev, MODE_CONFIG);
	if (res > 0) {
		mdelay(10);
		return res;
	}

	mdelay(10);

	if (mcp2515_configRate(spi_dev, canSpeed, clock)) {
		pr_info("Set rate failed\n");
		return res;
	}

	if (res == MCP2515_OK) {
		mcp2515_initCANBuffers(spi_dev);
		mcp2515_setRegister(spi_dev, MCP_CANINTE, MCP_RX0IF | MCP_RX1IF);

		mcp2515_modifyRegister(spi_dev, MCP_RXB0CTRL, MCP_RXB_RX_MASK |
					MCP_RXB_BUKT_MASK, MCP_RXB_RX_STDEXT |
					MCP_RXB_BUKT_MASK);
		mcp2515_modifyRegister(spi_dev, MCP_RXB1CTRL, MCP_RXB_RX_MASK,
					MCP_RXB_RX_STDEXT);

		res = setMode(spi_dev, MODE_NORMAL);
		if (res) {
			pr_err("Enter normal mode failed\n");
			return res;
		}
		mdelay(10);
	}

	return res;
}

void mcp2515_id_to_buf(uint8_t ext, unsigned long id,
			uint8_t *tbufdata)
{
	int canid;

	canid = (int)(id & 0x0FFFF);

	if (ext == 1) {
		tbufdata[MCP_EID0] = (uint8_t)(canid & 0xFF);
		tbufdata[MCP_EID8] = (uint8_t)(canid >> 8);
		canid = (int)(id >> 16);
		tbufdata[MCP_SIDL] = (uint8_t)(canid & 0x03);
		tbufdata[MCP_SIDL] += (uint8_t)((canid & 0x1C) << 3);
		tbufdata[MCP_SIDL] |= MCP_TXB_EXIDE_M;
		tbufdata[MCP_SIDH] = (uint8_t)(canid >> 5 );
	} else {
		tbufdata[MCP_SIDH] = (uint8_t)(canid >> 3 );
		tbufdata[MCP_SIDL] = (uint8_t)((canid & 0x07 ) << 5);
		tbufdata[MCP_EID0] = 0;
		tbufdata[MCP_EID8] = 0;
	}
}

void mcp2515_write_id(void *spi_dev, uint8_t mcp_addr, uint8_t ext, 
			unsigned long id)
{
	uint8_t tbufdata[4];

	mcp2515_id_to_buf(ext, id, tbufdata);
	mcp2515_setRegisterS(spi_dev, mcp_addr, tbufdata, 4);
}

void mcp2515_read_id(void *spi_dev, uint8_t mcp_addr, uint8_t *ext, 
			unsigned long *id)
{
	uint8_t tbufdata[4];

	*ext = 0;
	*id = 0;

	mcp2515_readRegisterS(spi_dev, mcp_addr, tbufdata, 4);

	*id = (tbufdata[MCP_SIDH] << 3) + (tbufdata[MCP_SIDL] >> 5);

	if ((tbufdata[MCP_SIDL] & MCP_TXB_EXIDE_M) ==  MCP_TXB_EXIDE_M) {
		*id = (*id << 2) + (tbufdata[MCP_SIDL] & 0x03);
		*id = (*id << 8) + tbufdata[MCP_EID8];
		*id = (*id << 8) + tbufdata[MCP_EID0];
		*ext = 1;
	}
}

void mcp2515_start_transmit(void *spi_dev, uint8_t mcp_addr)
{
	MCP2515_SELECT();
	SPI_Tx(spi_dev, txSidhToRTS(mcp_addr));
	MCP2515_UNSELECT();
}

void mcp2515_write_canMsg(void *spi_dev, uint8_t buffer_sidh_addr,
			unsigned long id, uint8_t ext, uint8_t rtrBit,
			uint8_t len, uint8_t *buf)
{
	uint8_t load_addr = txSidhToTxLoad(buffer_sidh_addr);

	uint8_t tbufdata[4];
	uint8_t dlc = len | ( rtrBit ? MCP_RTR_MASK : 0 ) ;
	uint8_t i;

	mcp2515_id_to_buf(ext, id, tbufdata);

	MCP2515_SELECT();
	SPI_Tx(spi_dev, load_addr);
	for (i = 0; i < 4; i++) {
		SPI_Tx(spi_dev, tbufdata[i]);
	}

	SPI_Tx(spi_dev, dlc);

	for (i = 0; i < len && i < CAN_MAX_CHAR_IN_MESSAGE; i++) {
		SPI_Tx(spi_dev, buf[i]);
	}

	MCP2515_UNSELECT();

	mcp2515_start_transmit(spi_dev, buffer_sidh_addr);
}

void mcp2515_read_canMsg(void *spi_dev, uint8_t buffer_load_addr,
			unsigned long *id, uint8_t *ext, uint8_t *rtrBit,
			uint8_t *len, uint8_t *buf)
{
	uint8_t tbufdata[4];
	uint8_t i;
	uint8_t pMsgSize;

	MCP2515_SELECT();
	SPI_Tx(spi_dev, buffer_load_addr);

	for (i = 0; i < 4; i++) {
		tbufdata[i] = SPI_Rx(spi_dev);
	}

	*id = (tbufdata[MCP_SIDH] << 3) + (tbufdata[MCP_SIDL] >> 5);
	*ext = 0;

	if ((tbufdata[MCP_SIDL] & MCP_TXB_EXIDE_M) ==  MCP_TXB_EXIDE_M) {
		*id = (*id << 2) + (tbufdata[MCP_SIDL] & 0x03);
		*id = (*id << 8) + tbufdata[MCP_EID8];
		*id = (*id << 8) + tbufdata[MCP_EID0];
		*ext = 1;
	}

	pMsgSize = SPI_Rx(spi_dev);
	*len = pMsgSize & MCP_DLC_MASK;
	*rtrBit = (pMsgSize & MCP_RTR_MASK) ? 1 : 0;

	for (i = 0; i < *len && i < CAN_MAX_CHAR_IN_MESSAGE; i++) {
		buf[i] = SPI_Rx(spi_dev);
	}

	MCP2515_UNSELECT();
}

uint8_t mcp2515_isTXBufFree(void *spi_dev, uint8_t *txbuf_n, uint8_t iBuf)
{
	*txbuf_n = 0x00;

	if (iBuf >= MCP_N_TXBUFFERS ||
		(mcp2515_readStatus(spi_dev) & txStatusPendingFlag(iBuf))!=0 )
		return MCP_ALLTXBUSY;
	*txbuf_n = txCtrlReg(iBuf) + 1;

	mcp2515_modifyRegister(spi_dev, MCP_CANINTF, txIfFlag(iBuf), 0);

	return MCP2515_OK;
}

uint8_t mcp2515_getNextFreeTXBuf(void *spi_dev, uint8_t *txbuf_n)
{
	uint8_t status = mcp2515_readStatus(spi_dev) & MCP_STAT_TX_PENDING_MASK;
	uint8_t i;

	*txbuf_n = 0x00;

	if (status == MCP_STAT_TX_PENDING_MASK)
		return MCP_ALLTXBUSY;

	for (i = 0; i < 3 - nReservedTx; i++) {
		if ((status & txStatusPendingFlag(i)) == 0) {
			*txbuf_n = txCtrlReg(i) + 1;
			mcp2515_modifyRegister(spi_dev, MCP_CANINTF, txIfFlag(i),
						0);
			return MCP2515_OK;
		}
	}

	return MCP_ALLTXBUSY;
}

uint8_t can_begin(void *spi_dev, uint8_t speedset, uint8_t clockset)
{
	uint8_t res = mcp2515_init(spi_dev, speedset, clockset);
	gpio_request(CS_PIN, "CS pin");

	return ((res == MCP2515_OK) ? CAN_OK : CAN_FAILINIT);
}

void enableTxInterrupt(void *spi_dev, bool enable)
{
	uint8_t interruptStatus = mcp2515_readRegister(spi_dev, MCP_CANINTE);

	if (enable) {
		interruptStatus |= MCP_TX_INT;
	} else {
		interruptStatus &= ~MCP_TX_INT;
	}

	mcp2515_setRegister(spi_dev, MCP_CANINTE, interruptStatus);
}

uint8_t init_Mask(void *spi_dev, uint8_t num, uint8_t ext, unsigned long ulData)
{
	uint8_t res = MCP2515_OK;

	res = mcp2515_setCANCTRL_Mode(spi_dev, MODE_CONFIG);
	if (res > 0) {
		pr_err("Enter setting mode failed\n");
		return res;
	}

	if (num == 0) {
		mcp2515_write_id(spi_dev, MCP_RXM0SIDH, ext, ulData);
	} else if (num == 1) {
		mcp2515_write_id(spi_dev, MCP_RXM1SIDH, ext, ulData);
	} else
		res = MCP2515_FAIL;

	res = mcp2515_setCANCTRL_Mode(spi_dev, mcpMode);
	if (res > 0) {
		pr_err("Enter normal mode failed\n");
		return res;
	}

	return res;
}

uint8_t init_Filt(void *spi_dev, uint8_t num, uint8_t ext, unsigned long ulData)
{
	uint8_t res = MCP2515_OK;

	res = mcp2515_setCANCTRL_Mode(spi_dev, MODE_CONFIG);
	if (res > 0) {
		pr_err("Enter setting mode failed\n");
		return res;
	}

	switch (num) {
	case 0:
		mcp2515_write_id(spi_dev, MCP_RXF0SIDH, ext, ulData);
		break;

	case 1:
		mcp2515_write_id(spi_dev, MCP_RXF1SIDH, ext, ulData);
		break;

	case 2:
		mcp2515_write_id(spi_dev, MCP_RXF2SIDH, ext, ulData);
		break;

	case 3:
		mcp2515_write_id(spi_dev, MCP_RXF3SIDH, ext, ulData);
		break;

	case 4:
		mcp2515_write_id(spi_dev, MCP_RXF4SIDH, ext, ulData);
		break;

	case 5:
		mcp2515_write_id(spi_dev, MCP_RXF5SIDH, ext, ulData);

	default:
		res = MCP2515_FAIL;
	}

	res = mcp2515_setCANCTRL_Mode(spi_dev, mcpMode);
	if (res > 0) {
		pr_err("Enter normal mode failed\n Set filter failed\n");
		return res;
	}

	mdelay(10);

	return res;
}

uint8_t sendMsgBuf1(void *spi_dev, uint8_t status, unsigned long id, uint8_t ext,
			uint8_t rtrBit, uint8_t len, uint8_t *buf)
{
	uint8_t txbuf_n = statusToTxSidh(status);

	if (txbuf_n == 0)
		return CAN_FAILTX;

	mcp2515_modifyRegister(spi_dev, MCP_CANINTF, status, 0);
	mcp2515_write_canMsg(spi_dev, txbuf_n, id, ext, rtrBit, len, buf);

	return CAN_OK;
}

uint8_t trySendMsgBuf(void *spi_dev, unsigned long id, uint8_t ext,
			uint8_t rtrBit, uint8_t len, uint8_t *buf,
			uint8_t iTxBuf)
{
	uint8_t txbuf_n;

	if (iTxBuf < MCP_N_TXBUFFERS) {
		if (mcp2515_isTXBufFree(spi_dev, &txbuf_n, iTxBuf) != MCP2515_OK)
			return CAN_FAILTX;
	} else {
		if (mcp2515_getNextFreeTXBuf(spi_dev, &txbuf_n) != MCP2515_OK)
			return CAN_FAILTX;
	}

	mcp2515_write_canMsg(spi_dev, txbuf_n, id, ext, rtrBit, len, buf);

	return CAN_OK;
}

uint8_t sendMsg(void *spi_dev, unsigned long id, uint8_t ext, uint8_t rtrBit,
		uint8_t len, uint8_t *buf, bool wait_sent)
{
	uint8_t res, res1, txbuf_n;
	int uiTimeOut = 0;

	can_id = id;
	ext_flg = ext;
	rtr = rtrBit;

	do {
		if (uiTimeOut > 0) 
			mdelay(10);
		res = mcp2515_getNextFreeTXBuf(spi_dev, &txbuf_n);
		uiTimeOut++;
	} while (res == MCP_ALLTXBUSY && (uiTimeOut < TIMEOUTVALUE));

	if (uiTimeOut == TIMEOUTVALUE) {
		return CAN_GETTXBFTIMEOUT;
	}

	mcp2515_write_canMsg(spi_dev, txbuf_n, id, ext, rtrBit, len, buf);

	if (wait_sent) {
		uiTimeOut = 0;
		do {
			if (uiTimeOut > 0)
				mdelay(10);
			uiTimeOut++;
			res1 = mcp2515_readRegister(spi_dev, txbuf_n - 1);
			res1 = res1 & 0x08;
		} while (res1 && (uiTimeOut < TIMEOUTVALUE));

		if (uiTimeOut == TIMEOUTVALUE)
			return CAN_SENDMSGTIMEOUT;
	}

	return CAN_OK;
}

uint8_t sendMsgBuf(void *spi_dev, unsigned long id, uint8_t ext, uint8_t rtrBit,
			uint8_t len, uint8_t *buf, bool wait_sent)
{
	return sendMsg(spi_dev, id, ext, 0, len, buf, wait_sent);
}

uint8_t readMsgBufID(void *spi_dev, uint8_t status, unsigned long *id,
		uint8_t *ext, uint8_t *rtrBit, uint8_t *len, uint8_t *buf)
{
	uint8_t rc = CAN_NOMSG;

	if (status & MCP_RX0IF) {
		mcp2515_read_canMsg(spi_dev, MCP_READ_RX0, id, ext, rtrBit,
					len, buf);
		rc = CAN_OK;
	} else if (status & MCP_RX1IF) {
		mcp2515_read_canMsg(spi_dev, MCP_READ_RX1, id, ext, rtrBit,
					len, buf);
		rc = CAN_OK;
	}

	if (rc == CAN_OK) {
		rtr= *rtrBit;
		ext_flg= *ext;
		can_id = *id;
	} else
		*len = 0;

	return rc;
}

uint8_t readMsgBuf(void *spi_dev, uint8_t *len, uint8_t buf[])
{
	return readMsgBufID(spi_dev, readRxTxStatus(spi_dev), &can_id, &ext_flg,
				&rtr, len, buf);
}

uint8_t readRxTxStatus(void *spi_dev)
{
	uint8_t ret;

	ret = (mcp2515_readStatus(spi_dev) & (MCP_STAT_TXIF_MASK | 
		MCP_STAT_RXIF_MASK ));

	ret = (ret & MCP_STAT_TX0IF ? MCP_TX0IF : 0) |
		(ret & MCP_STAT_TX1IF ? MCP_TX1IF : 0) |
		(ret & MCP_STAT_TX2IF ? MCP_TX2IF : 0) |
		(ret & MCP_STAT_RXIF_MASK);

	return ret;
}

uint8_t checkClearRxStatus(uint8_t *status)
{
	uint8_t ret;

	ret = *status & MCP_RX0IF;
	*status &= ~MCP_RX0IF;

	if (ret == 0) {
		ret = *status & MCP_RX1IF;
		*status &= ~MCP_RX1IF;
	}

	return ret;
}

uint8_t checkClearTxStatus(uint8_t *status, uint8_t iTxBuf)
{
	uint8_t ret;
	uint8_t i;

	if (iTxBuf < MCP_N_TXBUFFERS) {
		ret = *status & txIfFlag(iTxBuf);
		*status &= ~txIfFlag(iTxBuf);
	} else {
		ret = 0;
		for (i = 0; i < 3 - nReservedTx; i++) {
			ret = *status & txIfFlag(i);
			if (ret != 0) {
				*status &= ~txIfFlag(i);
				return ret;
			}
		}
	}

	return ret;
}

void clearBufferTransmitIfFlags(void *spi_dev, uint8_t flags)
{
	flags &= MCP_TX_INT;
	if (flags == 0) return;
	mcp2515_modifyRegister(spi_dev, MCP_CANINTF, flags, 0);

}

uint8_t checkReceive(void *spi_dev)
{
	uint8_t res;

	res = mcp2515_readStatus(spi_dev);
	pr_info("%s res = %d\n", __func__, res);

	return ((res & MCP_STAT_RXIF_MASK) ? CAN_MSGAVAIL : CAN_NOMSG);
}

uint8_t checkError(void *spi_dev)
{
	uint8_t eflg = mcp2515_readRegister(spi_dev, MCP_EFLG);
	return ((eflg & MCP_EFLG_ERRORMASK) ? CAN_CTRLERROR : CAN_OK);
}

unsigned long getCanId(void)
{
	return can_id;
}

uint8_t isRemoteRequest(void)
{
	return rtr;
}

uint8_t isExtendedFrame(void)
{
	return ext_flg;
}

bool mcpDigitalWrite(void *spi_dev, uint8_t pin, uint8_t mode)
{
	switch (pin) {
	case MCP_RX0BF:
		switch (mode) {
		case 1:
			mcp2515_modifyRegister(spi_dev, MCP_BFPCTRL, B0BFS,
						B0BFS);
			return true;
			break;
		default:
			mcp2515_modifyRegister(spi_dev, MCP_BFPCTRL, B0BFS, 0);
			return true;
		}
		break;
	case MCP_RX1BF:
		switch (mode) {
		case 1:
			mcp2515_modifyRegister(spi_dev, MCP_BFPCTRL, B1BFS,
						B1BFS);
			return true;
			break;
		default:
			mcp2515_modifyRegister(spi_dev, MCP_BFPCTRL, B1BFS, 0);
			return true;
		}
		break;
	default:
		return false;
	}
}
