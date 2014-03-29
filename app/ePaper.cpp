#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>

#include "SimpleGPIO.h"
#include "ePaper.h"
#include "SpiRAM.h"
#include "epd.h"
using namespace std;

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

//	SetPWM(5000,2500,0);

//	char buf[SPI_MAX_BUF];
//	OpenSpiDevice(&spiFile);
//	buf[0]=0x83;
//	for(int i=0;i<2000;i++)
//	{
//		write(spiFile,buf,1);
//		gpio_set_value(GPIO_RESET,LOW);
//		usleep(500);
//		gpio_set_value(GPIO_RESET,HIGH);
//		usleep(500);
//	}
//	CloseSpiDevice(&spiFile);

	EPaperInit();
	TestSpiRAM();
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
	gpio_export(GPIO_FLASH_CS);
	gpio_export(GPIO_EPD_CS);

	gpio_set_dir(GPIO_RESET,OUTPUT_PIN);
	gpio_set_dir(GPIO_BUSY,INPUT_PIN);
	gpio_set_dir(GPIO_POWER,OUTPUT_PIN);
	gpio_set_dir(GPIO_DISCHARGE,OUTPUT_PIN);
	gpio_set_dir(GPIO_BORDER,OUTPUT_PIN);
	gpio_set_dir(GPIO_FLASH_CS,OUTPUT_PIN);
	gpio_set_dir(GPIO_EPD_CS,OUTPUT_PIN);

	gpio_set_value(GPIO_RESET,LOW);
	gpio_set_value(GPIO_BORDER,LOW);
	gpio_set_value(GPIO_POWER,LOW);
	gpio_set_value(GPIO_DISCHARGE,LOW);
	gpio_set_value(GPIO_FLASH_CS,HIGH);
	gpio_set_value(GPIO_EPD_CS,HIGH);
}

void ReleaseGpio()
{
	gpio_unexport(GPIO_RESET);
	gpio_unexport(GPIO_BUSY);
	gpio_unexport(GPIO_POWER);
	gpio_unexport(GPIO_DISCHARGE);
	gpio_unexport(GPIO_BORDER);
	gpio_unexport(GPIO_FLASH_CS);
	gpio_unexport(GPIO_EPD_CS);
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
