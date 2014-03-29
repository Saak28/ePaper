#ifndef SPIRAM_H
#define SPIRAM_H

#define SYSFS_SPI_RAM_DIR		"/dev/spidev1.0"
#define SPI_MAX_BUF				1024

#define SRAMRead        0x03     //Read Command for SRAM
#define SRAMWrite       0x02     //Write Command for SRAM
#define SRAMRDSR        0x05     //Read the status register
#define SRAMWRSR        0x01     //Write the status register
#define SRAMByteMode    0x01
#define SRAMPageMode    0x81
#define	SRAMSeqMode     0x41
#define	SRAMPageSize    32
#define	DummyByte       0xFF

void	SpiRAM_Init(void);
int		OpenSpiRAM(int*);
void	CloseSpiRAM(int*);
int		WriteSpiRAM(unsigned char);
UINT8	ReadSpiRAM(void);
void	SpiRAMWriteStatusReg(UINT8);
UINT8	SpiRAMReadStatusReg(void);
void	SpiRAMCommand(unsigned int address,unsigned char RWCmd);
UINT8	SpiRAMWriteByte(unsigned int address,unsigned char WriteData);
UINT8	SpiRAMReadByte(unsigned int address);
UINT8	SpiRAMWritePage(unsigned int address, unsigned char *WriteData);
UINT8	SpiRAMReadPage(unsigned int address,unsigned char *ReadData);
UINT8	SpiRAMWriteSeq(unsigned int address, unsigned char *WriteData,unsigned int WriteCnt);
UINT8	SpiRAMReadSeq(unsigned int address,unsigned char *ReadData,unsigned int ReadCnt);
void	SpiRAMFill(UINT16 address,UINT16 len,UINT8 data);
void	TestSpiRAM(void);
void	ShowMem(UINT16 address,int len);

#endif // SPIRAM_H
