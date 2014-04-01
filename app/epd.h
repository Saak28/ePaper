#ifndef EPD_H
#define EPD_H

#define SYSFS_SPI_EPD_DIR			"/dev/spidev1.1"
#define SPI_EPD_MAX_BUF				1024
#define LINE_BUFFER_DATA_SIZE		111

#define EPD_144						0
#define EPD_200						1
#define EPD_270						2

// Using 2.7" as maximum supported size. 2.7" resolution is 264*176.
// 264/8=33Bytes per line(total 176 lines), but use 64Bytes as a base for future
// supporting larger size image.
// 64*176=11264 Bytes --> 12K Bytes per default image size  */
#define epd_image_size				(long)4*1024*3  //12k
#define epd_page_size()				(epd_image_size/32)    //memory access per page=32 bytes
#define getAddress(page)			(long)(epd_image_size*page)


// brief The definition for driving stage to compare with for getting Odd and Even data  */
#define BLACK0   (UINT8)(0x03) //  getting bit1 or bit0 as black color(11)
#define BLACK1   (UINT8)(0x0C) //  getting bit3 or bit2 as black color(11)
#define BLACK2   (UINT8)(0x30) //  getting bit5 or bit4 as black color(11)
#define BLACK3   (UINT8)(0xC0) //  getting bit7 or bit6 as black color(11)
#define WHITE0   (UINT8)(0x02) //  getting bit1 or bit0 as white color(10)
#define WHITE1   (UINT8)(0x08) //  getting bit3 or bit2 as white color(10)
#define WHITE2   (UINT8)(0x20) //  getting bit5 or bit4 as white color(10)
#define WHITE3   (UINT8)(0x80) //  getting bit7 or bit6 as white color(10)
#define NOTHING0 (UINT8)(0x01) //  getting bit1 or bit0 as nothing input(01)
#define NOTHING1 (UINT8)(0x04) //  getting bit3 or bit2 as nothing input(01)
#define NOTHING2 (UINT8)(0x10) //  getting bit5 or bit4 as nothing input(01)
#define NOTHING3 (UINT8)(0x40) //  getting bit7 or bit6 as nothing input(01)
#define NOTHING  (UINT8)(0x55) //  sending Nothing frame, 01=Nothing, 0101=0x5
#define G2_NOTHING  (UINT8)(0x00)

#define ALL_BLACK           (UINT8)(0xFF)
#define ALL_WHITE           (UINT8)(0xAA)
#define BORDER_BYTE_B       (UINT8)(0xFF)
#define BORDER_BYTE_W       (UINT8)(0xAA)
#define ERROR_BUSY          (UINT8)(0xF0)
#define ERROR_COG_ID        (UINT8)(0xF1)
#define ERROR_BREAKAGE      (UINT8)(0xF2)
#define ERROR_DC            (UINT8)(0xF3)
#define ERROR_CHARGEPUMP    (UINT8)(0xF4)
#define RES_OK              (UINT8)(0x00)

typedef void (*EPD_read_memory_handler)(UINT memory_address,UINT8 *target_buffer,UINT8 byte_length);

const UINT8 SCAN_TABLE[4]={0xC0,0x30,0x0C,0x03};

struct COG_144_line_data_t
{
	UINT8 border_byte;  //  Internal border_control, for 1.44" EPD only */
	UINT8 even[16]; //  1.44" even byte array */
	UINT8 scan[24]; //  1.44" scan byte array */
	UINT8 odd [16]; //  1.44" odd byte array */
} ;

struct COG_200_line_data_t
{
	UINT8 even[25]; //  2" even byte array */
	UINT8 scan[24]; //  2" scan byte array */
	UINT8 odd [25]; //  2" odd byte array */
	UINT8 dummy_data;	//  dummy byte 0x00 */
} ;

struct COG_270_line_data_t
{
	UINT8 even[33]; //  2.7" even byte array */
	UINT8 scan[44]; //  2.7" scan byte array */
	UINT8 odd [33]; //  2.7" odd byte array */
	UINT8 dummy_data;	//  dummy byte 0x00 */
} ;

typedef union
{
	union
	{
		struct COG_144_line_data_t line_data_for_144; //  line data structure of 1.44" EPD */
		struct COG_200_line_data_t line_data_for_200; //  line data structure of 2" EPD */
		struct COG_270_line_data_t line_data_for_270; //  line data structure of 2.7" EPD */
	} line_data_by_size; //  the line data of specific EPD size */
	UINT8 uint8[LINE_BUFFER_DATA_SIZE]; //  the maximum line buffer data size as length */
} COG_line_data_packet_type;

struct COG_parameters_t
{
	UINT8   channel_select[8]; //  the SPI register data of Channel Select */
	UINT8   voltage_level;     //  the SPI register data of Voltage Level */
	UINT16  horizontal_size;   //  the bytes of width of EPD */
	UINT16  vertical_size;     //  the bytes of height of EPD */
	UINT8   data_line_size;    //  Data + Scan + Dummy bytes */
	UINT16  frame_time_offset; //  the rest of frame time in a stage */
	UINT16  stage_time;        //  defined stage time */
} ;

const struct COG_parameters_t COG_parameters[3]=
{
	{
		// FOR 1.44"
		{0x00,0x00,0x00,0x00,0x00,0x0F,0xFF,0x00},
		0x03,
		(128/8),
		96,
		((((128+96)*2)/8)+1),
		0,
		480
	},
	{
		// For 2.0"
		{0x00,0x00,0x00,0x00,0x01,0xFF,0xE0,0x00},
		0x03,
		(200/8),
		96,
		((((200+96)*2)/8)+1),
		0,
		480
	},
	{
		// For 2.7"
		{0x00,0x00,0x00,0x7F,0xFF,0xFE,0x00,0x00},
		0x00,
		(264/8),
		176,
		((((264+176)*2)/8)+1),
		0,
		630
	}
};

enum Stage
{
	Stage1, // Inverse previous image
	Stage2, // White
	Stage3, // Inverse new image
	Stage4  // New image
};

void EpdInit(void);
void read_SRAM_handle(UINT memory_address,UINT8 *target_buffer,UINT8 byte_length);
void EpdGlobalUpdate(void);
void EpdPartialUpdate(void);
void EpdDisplayPartial(UINT8,UINT,UINT,EPD_read_memory_handler);
void EpdStoreScreen(void);
UINT8 EpdInitializeDriver(UINT8);
void EPD_image_data_globa_handle(UINT,UINT,EPD_read_memory_handler);
void EpdStageGlobalHandle(UINT16,UINT16,UINT16,UINT16,UINT,UINT8);
void epd_stage_partial_handle(UINT16,UINT16,UINT16,UINT16,UINT,UINT);
void epd_frame_global_handle(UINT16,UINT16,UINT16,UINT16,UINT,UINT8);
void EPD_image_data_partial_handle(UINT,UINT,EPD_read_memory_handler);
void epd_line_data_global_handle(UINT16,UINT16,UINT8 *,UINT8);
void epd_display_line_dummy_handle(void);
void EpdNothingFrame(void);
UINT8 Rotatebyte(UINT8);
void EpdBorderLine(UINT8);
int OpenSpiEPD(int*);
int EpdWriteSpi(UINT8);
UINT8 EpdReadSpiRAM(void);
void EpdDummyLine(UINT8);
void CloseSpiEPD(int*);
void EpdSpiSend(UINT8 register_index,UINT8 *register_data,UINT8 length);
void EpdSpiSendByte(UINT8 register_index,UINT8 register_data);
void EpdSetPower(bool);
void EpdCls(unsigned char);
int EpdWriteCmd(unsigned char);
int EpdWriteData(unsigned char);
void epd_frame_partial_handle(UINT16,UINT16,UINT16,UINT16,UINT,UINT);
void epd_line_data_partial_handle(UINT16,UINT16,UINT8 *,UINT8 *);
void EpdSetPixel(unsigned char,unsigned char,unsigned int);
void EpdTest(void);

extern long previous_image_address,new_image_address;

extern int GPIO_RESET;
extern int GPIO_BUSY;
extern int GPIO_POWER;
extern int GPIO_DISCHARGE;
extern int GPIO_BORDER;
extern int GPIO_FLASH_CS;
extern int GPIO_EPD_CS;

#endif // EPD_H
