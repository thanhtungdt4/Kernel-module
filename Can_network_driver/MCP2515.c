#include "MCP2515.h"
#include <linux/delay.h>
#include <linux/jiffies.h>

#define CS_PIN		24

id_reg_t idReg;
ctrl_status_t ctrlStatus;
ctrl_error_status_t errorStatus;

void gpio_set_pin(int gpio, int state)
{
	gpio_direction_output(gpio, 1);
	gpio_set_value(gpio, state);
}

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

void MCP2515_BitModify(void *spi_dev, uint8_t address, uint8_t mask,
			uint8_t data)
{
	gpio_set_pin(CS_PIN, 0);

	SPI_Tx(spi_dev, MCP2515_BIT_MOD);
	SPI_Tx(spi_dev, address);
	SPI_Tx(spi_dev, data);

	gpio_set_pin(CS_PIN, 1);
}

uint8_t MCP2515_GetRxStatus(void *spi_dev)
{
	uint8_t ret;

	gpio_set_pin(CS_PIN, 0);

	SPI_Tx(spi_dev, MCP2515_RX_STATUS);
	ret = SPI_Rx(spi_dev);

	gpio_set_pin(CS_PIN, 1);

	return ret;
}

uint8_t MCP2515_ReadStatus(void *spi_dev)
{
	uint8_t ret;

	gpio_set_pin(CS_PIN, 0);

	SPI_Tx(spi_dev, MCP2515_READ_STATUS);
	ret = SPI_Rx(spi_dev);

	gpio_set_pin(CS_PIN, 1);

	return ret;
}

void MCP2515_RequestToSend(void *spi_dev, uint8_t instruction)
{
	gpio_set_pin(CS_PIN, 0);

	SPI_Tx(spi_dev, instruction);

	gpio_set_pin(CS_PIN, 1);
}

void MCP2515_LoadTxBuffer(void *spi_dev, uint8_t instruction, uint8_t data)
{
	gpio_set_pin(CS_PIN, 0);

	SPI_Tx(spi_dev,instruction);
	SPI_Tx(spi_dev, data);

	gpio_set_pin(CS_PIN, 1);
}

void MCP2515_LoadTxSequence(void *spi_dev, uint8_t instruction, uint8_t *idReg,
				uint8_t dlc, uint8_t *data)
{
	gpio_set_pin(CS_PIN, 0);

	SPI_Tx(spi_dev,instruction);
	spi_write(spi_dev, idReg, 4);
	SPI_Tx(spi_dev, dlc);
	spi_write(spi_dev, data, dlc);

	gpio_set_pin(CS_PIN, 1);
}

void MCP2515_WriteByteSequence(void *spi_dev, uint8_t startAddress,
		uint8_t endAddress, uint8_t *data)
{
	gpio_set_pin(CS_PIN, 0);

	SPI_Tx(spi_dev,MCP2515_WRITE);
	SPI_Tx(spi_dev, startAddress);
	spi_write(spi_dev, data, (endAddress - startAddress + 1));

	gpio_set_pin(CS_PIN, 1);
}

void MCP2515_WriteByte(void *spi_dev, uint8_t address, uint8_t data)
{
	gpio_set_pin(CS_PIN, 0);

	SPI_Tx(spi_dev,MCP2515_WRITE);
	SPI_Tx(spi_dev, address);
	SPI_Tx(spi_dev, data);

	gpio_set_pin(CS_PIN, 1);
}

void MCP2515_ReadRxSequence(void *spi_dev, uint8_t instruction, uint8_t *data,
				uint8_t len)
{
	SPI_Tx(spi_dev, instruction);
	spi_read(spi_dev, data, len);
}

uint8_t MCP2515_ReadByte(void *spi_dev, uint8_t address)
{
	uint8_t ret = 0;

	gpio_set_pin(CS_PIN, 0);

	SPI_Tx(spi_dev, MCP2515_READ);
	SPI_Tx(spi_dev, address);
	ret = SPI_Rx(spi_dev);

	gpio_set_pin(CS_PIN, 1);

	return ret;
}

void MCP2515_Reset(void *spi_dev)
{
	SPI_Tx(spi_dev, MCP2515_RESET);
}

bool MCP2515_SetSleepMode(void *spi_dev)
{
	uint8_t loop = 10;

	MCP2515_WriteByte(spi_dev, MCP2515_CANCTRL, 0x20);

	do {
		if ((MCP2515_ReadByte(spi_dev, MCP2515_CANSTAT) & 0xE0) == 0x20)
			return true;
		loop--;
	} while (loop > 0);

	return false;
}

bool MCP2515_SetNormalMode(void *spi_dev)
{
	uint8_t loop = 10;
	uint8_t ret = 0;
	uint8_t ret2 = 0;

	MCP2515_WriteByte(spi_dev, MCP2515_CANCTRL, 0x00);
	ret2 = MCP2515_ReadByte(spi_dev, MCP2515_CANCTRL);
	pr_info("Tung: ret2 = %d\n", ret2);
	
	do {
		ret = MCP2515_ReadByte(spi_dev, MCP2515_CANSTAT);
		if ((ret & 0xE0) == 0x00) {
			pr_info("Tung: Set normal mode ok %d\n", ret);
			return true;
		} else {
			pr_err("Tung: Set normal mode failed %d\n", ret);
		}
		loop--;
	} while (loop > 0);

	return false;
}

bool MCP2515_SetConfigMode(void *spi_dev)
{
	uint8_t ret = 0;
	uint8_t ret2 = 0;
	unsigned long timeout;

	MCP2515_Reset(spi_dev);
	mdelay(100);

	timeout = jiffies + HZ;
	MCP2515_WriteByte(spi_dev, MCP2515_CANCTRL, 0x80);
	ret2 = MCP2515_ReadByte(spi_dev, MCP2515_CANCTRL);
	pr_info("Tung: ret2 = %d\n", ret2);

	while ((ret = MCP2515_ReadByte(spi_dev, MCP2515_CANSTAT) & 0xE0) != 0x80){
		if (time_after(jiffies, timeout)) {
			pr_err("Tung: MCP251x didn't enter in conf mode after reset: %d\n", ret);
			return false;
		}
	}
	pr_info("Tung: ret = %d\n", ret);

	return true;
}

void CANSPI_Sleep(void *spi_dev)
{
	MCP2515_BitModify(spi_dev, MCP2515_CANINTF, 0x40, 0x00);
	MCP2515_BitModify(spi_dev, MCP2515_CANINTE, 0x40, 0x40);
	MCP2515_SetSleepMode(spi_dev);
}

bool MCP2515_configRate(void *spi_dev, uint8_t canSpeed, uint8_t clock)
{
	uint8_t set = 1;
	uint8_t cfg1, cfg2, cfg3;

	switch (clock) {
	case MCP_16MHz:
		switch (canSpeed) {
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

		case CAN_1000KBPS:
			cfg1 = MCP_16MHz_1000kBPS_CFG1;
			cfg2 = MCP_16MHz_1000kBPS_CFG2;
			cfg3 = MCP_16MHz_1000kBPS_CFG3;
			break;
		default:
			set = 0;
			break;
		}
	case MCP_8MHz:
		switch (canSpeed) {
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

		//case CAN_80KBPS:
		//	cfg1 = MCP_8MHz_80kBPS_CFG1;
		//	cfg2 = MCP_8MHz_80kBPS_CFG2;
		//	cfg3 = MCP_8MHz_80kBPS_CFG3;
		//	break;

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
		MCP2515_WriteByte(spi_dev, MCP2515_CNF1, cfg1);
		MCP2515_WriteByte(spi_dev, MCP2515_CNF2, cfg2);
		MCP2515_WriteByte(spi_dev, MCP2515_CNF3, cfg3);
		return true;
	} else	
		return false;

}

bool CANSPI_Initialize(void *spi_dev, uint8_t canSpeed, uint8_t clock)
{
	RXF0 RXF0reg;
	RXF1 RXF1reg;
	RXF2 RXF2reg;
	RXF3 RXF3reg;
	RXF4 RXF4reg;
	RXF5 RXF5reg;
	RXM0 RXM0reg;
	RXM1 RXM1reg;

	RXM0reg.RXM0SIDH = 0x00;
	RXM0reg.RXM0SIDL = 0x00;
	RXM0reg.RXM0EID8 = 0x00;
	RXM0reg.RXM0EID0 = 0x00;

	RXM1reg.RXM1SIDH = 0x00;
	RXM1reg.RXM1SIDL = 0x00;
	RXM1reg.RXM1EID8 = 0x00;
	RXM1reg.RXM1EID0 = 0x00;

	RXF0reg.RXF0SIDH = 0x00;
	RXF0reg.RXF0SIDL = 0x00;
	RXF0reg.RXF0EID8 = 0x00;
	RXF0reg.RXF0EID0 = 0x00;

	RXF1reg.RXF1SIDH = 0x00;
	RXF1reg.RXF1SIDL = 0x08;
	RXF1reg.RXF1EID8 = 0x00;
	RXF1reg.RXF1EID0 = 0x00;

	RXF2reg.RXF2SIDH = 0x00;
	RXF2reg.RXF2SIDL = 0x00;
	RXF2reg.RXF2EID8 = 0x00;
	RXF2reg.RXF2EID0 = 0x00;

	RXF3reg.RXF3SIDH = 0x00;
	RXF3reg.RXF3SIDL = 0x00;
	RXF3reg.RXF3EID8 = 0x00;
	RXF3reg.RXF3EID0 = 0x00;

	RXF4reg.RXF4SIDH = 0x00;
	RXF4reg.RXF4SIDL = 0x00;
	RXF4reg.RXF4EID8 = 0x00;
	RXF4reg.RXF4EID0 = 0x00;

	RXF5reg.RXF5SIDH = 0x00;
	RXF5reg.RXF5SIDL = 0x08;
	RXF5reg.RXF5EID8 = 0x00;
	RXF5reg.RXF5EID0 = 0x00;

	gpio_request(CS_PIN, "CS pin");

	if (!MCP2515_SetConfigMode(spi_dev)) {
		pr_err("Tung: Set config mode failed\n");
		return false;
	}

	MCP2515_configRate(spi_dev, canSpeed, clock);
	mdelay(10);

	//MCP2515_WriteByteSequence(spi_dev, MCP2515_RXM0SIDH, MCP2515_RXM0EID0,
	//			&(RXM0reg.RXM0SIDH));
	//MCP2515_WriteByteSequence(spi_dev, MCP2515_RXM1SIDH, MCP2515_RXM1EID0,
	//			&(RXM1reg.RXM1SIDH));
	//MCP2515_WriteByteSequence(spi_dev, MCP2515_RXF0SIDH, MCP2515_RXF0EID0,
	//			&(RXF0reg.RXF0SIDH));
	//MCP2515_WriteByteSequence(spi_dev, MCP2515_RXF1SIDH, MCP2515_RXF1EID0,
	//			&(RXF1reg.RXF1SIDH));
	//MCP2515_WriteByteSequence(spi_dev, MCP2515_RXF2SIDH, MCP2515_RXF2EID0,
	//			&(RXF2reg.RXF2SIDH));
	//MCP2515_WriteByteSequence(spi_dev, MCP2515_RXF3SIDH, MCP2515_RXF3EID0,
	//			&(RXF3reg.RXF3SIDH));
	//MCP2515_WriteByteSequence(spi_dev, MCP2515_RXF4SIDH, MCP2515_RXF4EID0,
	//			&(RXF4reg.RXF4SIDH));
	//MCP2515_WriteByteSequence(spi_dev, MCP2515_RXF5SIDH, MCP2515_RXF5EID0,
	//			&(RXF5reg.RXF5SIDH));

	//MCP2515_WriteByte(spi_dev, MCP2515_RXB0CTRL, 0x04);
	//MCP2515_WriteByte(spi_dev, MCP2515_RXB1CTRL, 0x01);

	MCP2515_BitModify(spi_dev, MCP2515_RXB0CTRL, (MCP_RXB_RX_MASK | MCP_RXB_BUKT_MASK),
			 (MCP_RXB_RX_ANY | MCP_RXB_BUKT_MASK));

	MCP2515_BitModify(spi_dev, MCP2515_RXB1CTRL, MCP_RXB_RX_MASK,
				MCP_RXB_RX_ANY);


	if (!MCP2515_SetNormalMode(spi_dev)) {
		pr_err("Tung: Set normal mode failed\n");
		return false;
	}

	return true;
}

uint32_t convertReg2ExtendedCANid(uint8_t tempRXBn_EIDH, uint8_t tempRXBn_EIDL,
				uint8_t tempRXBn_SIDH, uint8_t tempRXBn_SIDL)
{
	uint32_t returnValue = 0;
	uint32_t ConvertedID = 0;
	uint8_t CAN_standardLo_ID_lo2bits;
	uint8_t CAN_standardLo_ID_hi3bits;

	CAN_standardLo_ID_lo2bits = (tempRXBn_SIDL & 0x03);
	CAN_standardLo_ID_hi3bits = (tempRXBn_SIDL >> 5);
	ConvertedID = (tempRXBn_SIDH << 3);
	ConvertedID = ConvertedID + CAN_standardLo_ID_hi3bits;
	ConvertedID = (ConvertedID << 2);
	ConvertedID = ConvertedID + CAN_standardLo_ID_lo2bits;
	ConvertedID = (ConvertedID << 8);
	ConvertedID = ConvertedID + tempRXBn_EIDH;
	ConvertedID = (ConvertedID << 8);
	ConvertedID = ConvertedID + tempRXBn_EIDL;
	returnValue = ConvertedID;

	return returnValue;
}

uint32_t convertReg2StandardCANid(uint8_t tempRXBn_SIDH, uint8_t tempRXBn_SIDL)
{
	uint32_t returnValue = 0;
	uint32_t ConvertedID;

	ConvertedID = (tempRXBn_SIDH << 3);
	ConvertedID = ConvertedID + (tempRXBn_SIDL >> 5);
	returnValue = ConvertedID;

	return returnValue;
}

void convertCANid2Reg(uint8_t tempPassedInID, uint8_t canIdType,
				id_reg_t *passedIdReg)
{
	uint8_t wipSIDL = 0;

	if (canIdType == dEXTENDED_CAN_MSG_ID_2_0B) {
		passedIdReg->tempEID0 = 0xFF & tempPassedInID;
		tempPassedInID = tempPassedInID >> 8;

		passedIdReg->tempEID8 = 0xFF & tempPassedInID;
		tempPassedInID = tempPassedInID >> 8;

		wipSIDL = 0x03 & tempPassedInID;
		tempPassedInID = tempPassedInID << 3;
		wipSIDL = (0xE0 & tempPassedInID) + wipSIDL;
		wipSIDL = wipSIDL + 0x08;
		passedIdReg->tempSIDL = 0xEB & wipSIDL;

		tempPassedInID = tempPassedInID >> 8;
		passedIdReg->tempSIDH = 0xFF & tempPassedInID;
	} else {
		passedIdReg->tempEID8 = 0;
		passedIdReg->tempEID0 = 0;
		tempPassedInID = tempPassedInID << 5;
		passedIdReg->tempSIDL = 0xFF & tempPassedInID;
		tempPassedInID = tempPassedInID >> 8;
		passedIdReg->tempSIDH = 0xFF & tempPassedInID;
	}
}

uint8_t CANSPI_Transmit(void *spi_dev, uCAN_MSG *tempCanMsg)
{
	uint8_t ret = 0;

	idReg.tempSIDH = 0;
	idReg.tempSIDL = 0;
	idReg.tempEID8 = 0;
	idReg.tempEID0 = 0;

	ctrlStatus.ctrl_status = MCP2515_ReadStatus(spi_dev);
	pr_info("Tung: read status in transmit is %d\n", ctrlStatus.ctrl_status);

	if (ctrlStatus.TXB0REQ != 1) {
		convertCANid2Reg(tempCanMsg->frame.id, tempCanMsg->frame.idType,
					&idReg);
		MCP2515_LoadTxSequence(spi_dev, MCP2515_LOAD_TXB0SIDH,
				&(idReg.tempSIDH), tempCanMsg->frame.dlc,
				&(tempCanMsg->frame.data0));
		MCP2515_RequestToSend(spi_dev, MCP2515_RTS_TX0);
		ret = 1;
	} else if (ctrlStatus.TXB1REQ != 1) {
		convertCANid2Reg(tempCanMsg->frame.id, tempCanMsg->frame.idType,
					&idReg);
		MCP2515_LoadTxSequence(spi_dev, MCP2515_LOAD_TXB1SIDH,
				&(idReg.tempSIDH), tempCanMsg->frame.dlc,
				&(tempCanMsg->frame.data0));
		MCP2515_RequestToSend(spi_dev, MCP2515_RTS_TX1);
		ret = 1;
	} else if (ctrlStatus.TXB2REQ != 1) {
		convertCANid2Reg(tempCanMsg->frame.id, tempCanMsg->frame.idType,
					&idReg);
		MCP2515_LoadTxSequence(spi_dev, MCP2515_LOAD_TXB2SIDH,
				&(idReg.tempSIDH), tempCanMsg->frame.dlc,
				&(tempCanMsg->frame.data0));
		MCP2515_RequestToSend(spi_dev, MCP2515_RTS_TX2);
		ret = 1;
	}

	return ret;
}

uint8_t CANSPI_Receive(void *spi_dev, uCAN_MSG *tempCanMsg)
{
	uint8_t ret = 0;
	rx_reg_t rxReg;
	ctrl_rx_status_t rxStatus;

	rxStatus.ctrl_rx_status = MCP2515_GetRxStatus(spi_dev);
	pr_info("Tung: RX status is %d\n", rxStatus.ctrl_rx_status);

	if (rxStatus.rxBuffer != 0) {
		if ((rxStatus.rxBuffer == MSG_IN_RXB0) | 
		(rxStatus.rxBuffer == MSG_IN_BOTH_BUFFERS)) {
			MCP2515_ReadRxSequence(spi_dev, MCP2515_READ_RXB0SIDH,
					rxReg.rx_reg_array,
					sizeof(rxReg.rx_reg_array));
		} else if (rxStatus.rxBuffer == MSG_IN_RXB1) {
			MCP2515_ReadRxSequence(spi_dev, MCP2515_READ_RXB1SIDH,
					rxReg.rx_reg_array,
					sizeof(rxReg.rx_reg_array));
		}

		if (rxStatus.msgType == dEXTENDED_CAN_MSG_ID_2_0B) {
			tempCanMsg->frame.idType = (uint8_t)dEXTENDED_CAN_MSG_ID_2_0B;
			tempCanMsg->frame.id = convertReg2ExtendedCANid(rxReg.RXBnEID8,
						rxReg.RXBnEID0, rxReg.RXBnSIDH,
						rxReg.RXBnSIDL);
		} else {
			tempCanMsg->frame.idType = (uint8_t) dSTANDARD_CAN_MSG_ID_2_0B;
			tempCanMsg->frame.id = convertReg2StandardCANid(rxReg.RXBnSIDH,
						rxReg.RXBnSIDL);
		}

		tempCanMsg->frame.dlc   = rxReg.RXBnDLC;
		tempCanMsg->frame.data0 = rxReg.RXBnD0;
		tempCanMsg->frame.data1 = rxReg.RXBnD1;
		tempCanMsg->frame.data2 = rxReg.RXBnD2;
		tempCanMsg->frame.data3 = rxReg.RXBnD3;
		tempCanMsg->frame.data4 = rxReg.RXBnD4;
		tempCanMsg->frame.data5 = rxReg.RXBnD5;
		tempCanMsg->frame.data6 = rxReg.RXBnD6;
		tempCanMsg->frame.data7 = rxReg.RXBnD7;

		pr_info("Tung: data0 is %d\n", rxReg.RXBnD0);
		pr_info("Tung: data1 is %d\n", rxReg.RXBnD1);
		pr_info("Tung: data2 is %d\n", rxReg.RXBnD2);

		ret = 1;
	}

	return ret;
}

uint8_t CANSPI_messagesInBuffer(void *spi_dev)
{
	uint8_t messageCount = 0;

	ctrlStatus.ctrl_status = MCP2515_ReadStatus(spi_dev);
	if (ctrlStatus.RX0IF != 0) {
		messageCount++;
	}

	if (ctrlStatus.RX1IF != 0) {
		messageCount++;
	}

	return messageCount;
}

uint8_t CANSPI_isBussOff(void *spi_dev)
{
	uint8_t ret = 0;

	errorStatus.error_flag_reg = MCP2515_ReadByte(spi_dev, MCP2515_EFLG);
	if (errorStatus.TXBO == 1)
		ret = 1;

	return ret;
}

uint8_t CANSPI_isRxErrorPassive(void *spi_dev)
{
	uint8_t ret = 0;

	errorStatus.error_flag_reg = MCP2515_ReadByte(spi_dev, MCP2515_EFLG);
	if (errorStatus.RXEP == 1)
		ret = 1;

	return ret;
}

uint8_t CANSPI_isTxErrorPassive(void *spi_dev)
{
	uint8_t ret = 0;

	errorStatus.error_flag_reg = MCP2515_ReadByte(spi_dev, MCP2515_EFLG);
	if (errorStatus.TXEP == 1)
		ret = 1;

	return ret;
}

