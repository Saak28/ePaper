#ifndef EPD_H
#define EPD_H

#define SYSFS_SPI_EPD_DIR			"/dev/spidev1.1"
#define SPI_EPD_MAX_BUF				1024

void SpiEPD_Init(void);
int OpenSpiEPD(int*);
void CloseSpiEPD(int*);
int WriteSpiEPD(unsigned char);
void EPaperSetPower(bool);
int EPaperInit(void);
void EPaperCls(unsigned char);
int EPaperWriteCmd(unsigned char);
int EPaperWriteData(unsigned char);
void EPaperSetPixel(unsigned char,unsigned char,unsigned int);

extern int GPIO_RESET;
extern int GPIO_BUSY;
extern int GPIO_POWER;
extern int GPIO_DISCHARGE;
extern int GPIO_BORDER;
extern int GPIO_FLASH_CS;
extern int GPIO_EPD_CS;

#endif // EPD_H
