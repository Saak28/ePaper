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
#include "epd.h"
#include "SpiRAM.h"

using namespace std;

int spiRamFile;

void SpiRAM_Init()
{
	OpenSpiRAM(&spiRamFile);
	gpio_set_value(GPIO_FLASH_CS,HIGH);
}

int OpenSpiRAM(int *spiFile)
{
	*spiFile=open(SYSFS_SPI_RAM_DIR,O_RDWR);
	if(spiFile<0)
	{
		perror("WriteSpiRAM");
		return 0;
	}
	return 1;
}

void CloseSpiRAM(int *spiFile)
{
	close(*spiFile);
}

int WriteSpiRAM(unsigned char data)
{
	char buf[SPI_MAX_BUF];

	buf[0]=data;
	write(spiRamFile,buf,1);
	return 1;
}

UINT8 ReadSpiRAM()
{
	char buf[SPI_MAX_BUF];
	read(spiRamFile,buf,1);
	return buf[0];
}

void SpiRAMWriteStatusReg(UINT8 RegValue)
{
	char buf[SPI_MAX_BUF];
	gpio_set_value(GPIO_FLASH_CS,LOW);
	buf[0]=SRAMWRSR;
	buf[1]=RegValue;
	write(spiRamFile,buf,2);
	gpio_set_value(GPIO_FLASH_CS,HIGH);
}

UINT8 SpiRAMReadStatusReg()
{
	UINT8 ReadData=0;
	gpio_set_value(GPIO_FLASH_CS,LOW);
	WriteSpiRAM(SRAMRDSR);
	ReadData=ReadSpiRAM();
	gpio_set_value(GPIO_FLASH_CS,HIGH);
	return ReadData;
}

void SpiRAMCommand(unsigned int address,unsigned char RWCmd)
{
	char buf[SPI_MAX_BUF];
	buf[0]=RWCmd;
	buf[1]=(UINT8)(address >> 8);
	buf[2]=(UINT8)(address);
	write(spiRamFile,buf,3);
}

UINT8 SpiRAMWriteByte(unsigned int address,unsigned char WriteData)
{
	SpiRAMWriteStatusReg(SRAMByteMode);
	gpio_set_value(GPIO_FLASH_CS,LOW);
	SpiRAMCommand(address,SRAMWrite);	//Send Write command to SRAM along with address
	WriteSpiRAM(WriteData);				//Send Data to be written to SRAM
	gpio_set_value(GPIO_FLASH_CS,HIGH);
	return(0);							//Return non -ve number indicating success
}

UINT8 SpiRAMReadByte(unsigned int address)
{
	unsigned char ReadData;
	SpiRAMWriteStatusReg(SRAMByteMode);
	gpio_set_value(GPIO_FLASH_CS,LOW);
	SpiRAMCommand(address,SRAMRead);	//Send Read command to SRAM along with address
	ReadData=ReadSpiRAM();
	gpio_set_value(GPIO_FLASH_CS,HIGH);
	return(ReadData);
}

UINT8 SpiRAMWritePage(unsigned int address, unsigned char *WriteData)
{
	unsigned char WriteCnt;
	SpiRAMWriteStatusReg(SRAMPageMode);
	gpio_set_value(GPIO_FLASH_CS,LOW);
	SpiRAMCommand(address,SRAMWrite);			//Send Write command to SRAM along with address
	WriteCnt=write(spiRamFile,WriteData,SRAMPageSize);
	gpio_set_value(GPIO_FLASH_CS,HIGH);
	return(WriteCnt);							//Return no# of bytes written to SRAM
}

UINT8 SpiRAMReadPage(unsigned int address,unsigned char *ReadData)
{
	unsigned char ReadCnt;
	SpiRAMWriteStatusReg(SRAMPageMode);
	gpio_set_value(GPIO_FLASH_CS,LOW);
	SpiRAMCommand(address,SRAMRead);			//Send Read command to SRAM along with address
	ReadCnt=read(spiRamFile,ReadData,SRAMPageSize);
	gpio_set_value(GPIO_FLASH_CS,HIGH);
	return(ReadCnt);			//Return no# of bytes read from SRAM
}

UINT8 SpiRAMWriteSeq(unsigned int address, unsigned char *WriteData,unsigned int WriteCnt)
{
	SpiRAMWriteStatusReg(SRAMSeqMode);
	gpio_set_value(GPIO_FLASH_CS,LOW);

	SpiRAMCommand(address,SRAMWrite);			//Send Write command to SRAM along with address
	//Send Data to be written to SRAM
	for(;WriteCnt>0;WriteCnt--)
		WriteSpiRAM(*WriteData++);
	gpio_set_value(GPIO_FLASH_CS,HIGH);
	return(0);									//Return non -ve nuber indicating success
}

UINT8 SpiRAMReadSeq(unsigned int address,unsigned char *ReadData,unsigned int ReadCnt)
{
	SpiRAMWriteStatusReg(SRAMSeqMode);
	gpio_set_value(GPIO_FLASH_CS,LOW);

	SpiRAMCommand(address,SRAMRead);			//Send Read command to SRAM along with address
	for(;ReadCnt>0;ReadCnt--)
		*ReadData++=ReadSpiRAM();
	gpio_set_value(GPIO_FLASH_CS,HIGH);
	return(0);									//Return non -ve nuber indicating success
}

void SpiRAMFillPage(UINT16 address,UINT16 len,UINT8 data)
{
	UINT8 buf[SRAMPageSize];
	UINT8 bufCmd[SRAMPageSize];

	for(int i=0;i<SRAMPageSize;i++)
		buf[i]=data;

	SpiRAMWriteStatusReg(SRAMSeqMode);
	gpio_set_value(GPIO_FLASH_CS,LOW);
	bufCmd[0]=SRAMWrite;
	bufCmd[1]=(UINT8)((address)>>8);
	bufCmd[2]=(UINT8)(address);
	write(spiRamFile,bufCmd,3);
	for(int i=0;i<len;i+=SRAMPageSize)
		write(spiRamFile,buf,SRAMPageSize);
	gpio_set_value(GPIO_FLASH_CS,HIGH);
}

void TestSpiRAM()
{
	char str[128];

	UINT8 buf[SPI_MAX_BUF];

	cout << "TestRamSPI..." << endl;

	SpiRAMFillPage(0x0000,SPI_RAM_SIZE,0xFF);
	usleep(10000);
	ShowMem(0x0000,1024);
	cout << endl;

	SpiRAMWriteByte(0x0000,0x28);
	for(int i=0;i<32;i++)
		buf[i]=i;
	cout << "SpiRAMWritePage..." << endl;
	SpiRAMWritePage(0x0020,buf);
	ShowMem(0x0000,256);

//	for(int i=0;i<0x80;i++)
//		buf[i]=i;
//	SpiRAMWriteSeq(0x0040,buf,0x80);
//	ShowMem(0x0000,256);
}

void ShowMem(UINT16 address,int len)
{
	UINT16 i,j,k;
	UINT8 c;
	UINT8 tab[256];
	char str[128];

	for(i=0;i<len;i+=32)
	{
		k=0;
		SpiRAMReadPage(address+i,tab);
//		SpiRAMReadSeq(address+i,tab,32);

		sprintf(str,"%04X:",i);
		cout << str;
		for(j=0;j<32;j++)
		{
			sprintf(str,"%02X ",tab[k+j]);
			cout << str;
		}
		cout << " ";
		for(j=0;j<32;j++)
		{
			c=tab[k+j];
			if(c<' ')	c='.';
			if(c>'z')	c='.';
			sprintf(str,"%c",c);
			cout << str;
		}
		k+=32;
		cout << endl;
	}
}
