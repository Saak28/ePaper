#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <iostream>

#include "SimpleGPIO.h"
#include "ePaper.h"
#include "SpiRAM.h"
#include "epd.h"

using namespace std;

int spiEPDFile;
int GPIO_RESET=60;
int GPIO_BUSY=49;
int GPIO_POWER=48;
int GPIO_DISCHARGE=50;
int GPIO_BORDER=51;
int GPIO_FLASH_CS=113;
int GPIO_EPD_CS=7;


void SpiEPD_Init()
{
	OpenSpiEPD(&spiEPDFile);
}

int OpenSpiEPD(int *spiFile)
{
	*spiFile=open(SYSFS_SPI_EPD_DIR,O_RDWR);
	if(spiFile<0)
	{
		perror("WriteSpiEPD");
		return 0;
	}
	return 1;
}

void CloseSpiEPD(int *spiFile)
{
	close(*spiFile);
}

int WriteSpiEPD(unsigned char data)
{
	char buf[SPI_EPD_MAX_BUF];

	int fd=open(SYSFS_SPI_EPD_DIR,O_RDWR);
	if(fd<0)
	{
		perror("WriteSpiEPD");
		return 0;
	}
	buf[0]=data;
	write(fd,buf,1);
	close(fd);
	return 1;
}

void EPaperSetPower(bool mode)
{
	if(mode)
	{
		gpio_set_value(GPIO_DISCHARGE,LOW);
		gpio_set_value(GPIO_RESET,LOW);
		OpenSpiEPD(&spiEPDFile);
		SetPWM(5000,2500,0);
		gpio_set_value(GPIO_POWER,HIGH);
		usleep(10000);
		gpio_set_value(GPIO_BORDER,HIGH);
		gpio_set_value(GPIO_RESET,HIGH);
		usleep(5000);
		gpio_set_value(GPIO_RESET,LOW);
		usleep(5000);
		gpio_set_value(GPIO_RESET,HIGH);
	}
	else
	{

	}
}

int EPaperInit()
{
	EPaperSetPower(true);	// Power on COG Driver
	SpiRAM_Init();

//	EPaperInitialize_driver(EPD_type_index);	// Initialize COG Driver

//	usleep(120);
//	gpio_set_value(GPIO_RESET,HIGH);

//	EPaperWriteCmd(CMD_SET_COMMAND_LOCK);
//	EPaperWriteData(0x12);	// Unlock Basic Commands (0x12/0x16)


	return 1;
}

void EPaperCls(unsigned char fillData)
{
	EPaperWriteCmd(CMD_SET_COLUMN_ADDRESS);
	EPaperWriteData(0x1C);

	EPaperWriteCmd(CMD_SET_ROW_ADDRESS);
	EPaperWriteData(0x00);

	EPaperWriteCmd(CMD_WRITE_RAM_CMD);
	gpio_set_value(GPIO_BUSY,HIGH);

	char buf[SPI_EPD_MAX_BUF];
	int spiFile;
	if(!OpenSpiEPD(&spiEPDFile))
		return;

	for(int i=0;i<SPI_EPD_MAX_BUF;i++)
		buf[i]=fillData;;
	for(int j=0;j<8;j++)
		write(spiEPDFile,buf,SPI_EPD_MAX_BUF);
	CloseSpiEPD(&spiEPDFile);
}

int EPaperWriteCmd(unsigned char cmd)
{
	gpio_set_value(GPIO_BUSY,LOW);
	return WriteSpiEPD(cmd);
}

int EPaperWriteData(unsigned char data)
{
	gpio_set_value(GPIO_BUSY,HIGH);
	return WriteSpiEPD(data);
}

void EPaperSetPixel(unsigned char x,unsigned char y,unsigned int color)
{
	EPaperWriteCmd(CMD_SET_COLUMN_ADDRESS);
	EPaperWriteData(0x1C+(x>>2));

	EPaperWriteCmd(CMD_SET_ROW_ADDRESS);
	EPaperWriteData(y);

	EPaperWriteCmd(CMD_WRITE_RAM_CMD);
	EPaperWriteData(color>>8);
	EPaperWriteData(color);
}
