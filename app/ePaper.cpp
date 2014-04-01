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
	EpdInit();

	EpdSetPower(true);						// Power on COG Driver
	SpiRAMFillPage(0x0000,SPI_RAM_SIZE,0xFF);
	SpiRAMWriteByte(0x0000,0xAA);
	ShowMem(0x0000,1024);
	EpdInitializeDriver(EPD_270);
	EpdGlobalUpdate();
//	EpdTest();

	SpiRAMFillPage(new_image_address,epd_image_size/2,0xFE);
	EpdPartialUpdate();
	int k=0x80;
	for(int i=0;i<8;i++)
	{
		SpiRAMFillPage(new_image_address+2*(epd_image_size/8),epd_image_size/8,~k);
		k>>=1;
		EpdPartialUpdate();
	}
//	EpdGlobalUpdate();

//	TestSpiRAM();

//	int k,data;
//	for(int i=0;i<128;i++)
//	{
//		k=0x80;
//		data=0;
//		for(int j=0;j<8;j++)
//		{
//			data|=k;
//			k>>=1;
//			SpiRAMWriteByte(new_image_address+i,~data);
//			EpdPartialUpdate();
//		}
//	}
//	usleep(1000000);
//	SpiRAMWriteByte(new_image_address,0x55);
//	EpdPartialUpdate();

//	usleep(1000000);
//	SpiRAMWriteByte(new_image_address,0xAA);
//	EpdPartialUpdate();

//	usleep(1000000);
//	SpiRAMWriteByte(new_image_address,0x81);
//	EpdPartialUpdate();

//	usleep(1000000);
//	SpiRAMWriteByte(new_image_address,0xC3);
//	EpdPartialUpdate();

	EpdSetPower(false);
	cout << "Done..." << endl;
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
