#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include "SimpleGPIO.h"
#include "ePaper.h"
using namespace std;

unsigned int GPIO_RESET=60;
unsigned int GPIO_BUSY=49;
unsigned int GPIO_POWER=48;
unsigned int GPIO_DISCHARGE=50;
unsigned int GPIO_BORDER=51;
int spiFile;

int main(int argc, char *argv[])
{
	cout << "ePaper Display Linux Test Application..." << endl;

	InitGpio();
	EPaperInit();
//	EPaperCls(0x44);
//	EPaperCls(0x00);
	
//	EPaperSetPixel(0,0,0xF000);
//	EPaperSetPixel(252,0,0x000F);
//	EPaperSetPixel(0,63,0xF000);
//	EPaperSetPixel(252,63,0x000F);

//	for(int i=0;i<16;i++)
//		EPaperSetPixel(127,i,0x1000*i);

	SetPWM(5000,2500,0);

	char buf[SPI_MAX_BUF];
	OpenSpiDevice(&spiFile);
	buf[0]=0x83;
	for(int i=0;i<2000;i++)
	{
		write(spiFile,buf,1);
		gpio_set_value(GPIO_RESET,LOW);
		usleep(500);
		gpio_set_value(GPIO_RESET,HIGH);
		usleep(500);
	}
	CloseSpiDevice(&spiFile);

//	usleep(2000000);
	SetPWM(5000,5000,1);
	ReleaseGpio();
	
	return 0;
}

void InitGpio()
{
	gpio_export(GPIO_RESET);
	gpio_export(GPIO_BUSY);
	gpio_export(GPIO_POWER);
	gpio_export(GPIO_DISCHARGE);
	gpio_export(GPIO_BORDER);

	gpio_set_dir(GPIO_RESET,OUTPUT_PIN);
	gpio_set_dir(GPIO_BUSY,INPUT_PIN);
	gpio_set_dir(GPIO_POWER,OUTPUT_PIN);
	gpio_set_dir(GPIO_DISCHARGE,OUTPUT_PIN);
	gpio_set_dir(GPIO_BORDER,OUTPUT_PIN);

	gpio_set_value(GPIO_RESET,LOW);
	gpio_set_value(GPIO_BORDER,LOW);
	gpio_set_value(GPIO_POWER,LOW);
	gpio_set_value(GPIO_DISCHARGE,LOW);
}

void ReleaseGpio()
{
	gpio_unexport(GPIO_RESET);
	gpio_unexport(GPIO_BUSY);
	gpio_unexport(GPIO_POWER);
	gpio_unexport(GPIO_DISCHARGE);
	gpio_unexport(GPIO_BORDER);
}

int SetPWM(unsigned int period,unsigned int duty,int polarity)
{
	int fd, len;
	char buf[MAX_BUF];

	cout << "PWM..." << period <<" " << duty <<" " << polarity <<" " << endl;

	fd=open(PWM_DIR "/duty",O_WRONLY);
	if(fd<0)	return -1;
	len=snprintf(buf,sizeof(buf),"%d",duty);
	write(fd, buf, len);
	close(fd);

	fd=open(PWM_DIR "/period",O_WRONLY);
	if(fd<0)	return -1;
	len=snprintf(buf,sizeof(buf),"%d",period);
	write(fd, buf, len);
	close(fd);

	fd=open(PWM_DIR "/polarity",O_WRONLY);
	if(fd<0)	return -1;
	len=snprintf(buf,sizeof(buf),"%d",polarity);
	write(fd, buf, len);
	close(fd);

	return 0;
}

void EPaperSetPower(bool mode)
{
	if(mode)
	{
		gpio_set_value(GPIO_DISCHARGE,LOW);
		gpio_set_value(GPIO_RESET,LOW);
		OpenSpiDevice(&spiFile);
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
	EPaperInitializeDriver(EPD_type_index);	// Initialize COG Driver

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

	char buf[SPI_MAX_BUF];
	int spiFile;	
	if(!OpenSpiDevice(&spiFile))
		return;

	for(int i=0;i<SPI_MAX_BUF;i++)
		buf[i]=fillData;;
	for(int j=0;j<8;j++)
		write(spiFile,buf,SPI_MAX_BUF);
	CloseSpiDevice(&spiFile);
}

int EPaperWriteCmd(unsigned char cmd)
{
	gpio_set_value(GPIO_BUSY,LOW);
	return WriteSpi(cmd);
}

int EPaperWriteData(unsigned char data)
{
	gpio_set_value(GPIO_BUSY,HIGH);
	return WriteSpi(data);
}

int WriteSpi(unsigned char data)
{
	char buf[SPI_MAX_BUF];

	int fd=open(SYSFS_SPI_DIR,O_WRONLY);
	if(fd<0)
	{
		perror("WriteSpi");
		return 0;
	}
	buf[0]=data;
	write(fd,buf,1);
	close(fd);
	return 1;
}

int OpenSpiDevice(int *spiFile)
{
	*spiFile=open(SYSFS_SPI_DIR,O_WRONLY);
	if(spiFile<0)
	{
		perror("WriteSpi");
		return 0;
	}
	return 1;
}

void CloseSpiDevice(int *spiFile)
{
	close(*spiFile);
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
