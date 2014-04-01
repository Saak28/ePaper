#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <iostream>

#include "ePaper.h"
#include "SimpleGPIO.h"
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

long cur_image_index=0;
long previous_image_address,new_image_address;
UINT8 use_EPD_type_index;

static EPD_read_memory_handler _On_EPD_read_handle=NULL;

void EpdInit()
{
	// Init IO
	gpio_set_value(GPIO_POWER,LOW);
	gpio_set_value(GPIO_EPD_CS,LOW);
	gpio_set_value(GPIO_RESET,LOW);
	gpio_set_value(GPIO_DISCHARGE,LOW);
	gpio_set_value(GPIO_BORDER,LOW);
	SetPWM(5000,5000,1);
	SpiRAM_Init();
	gpio_set_value(GPIO_POWER,HIGH);

	OpenSpiEPD(&spiEPDFile);

	cur_image_index=0;
	new_image_address=getAddress(cur_image_index);
	previous_image_address=getAddress((cur_image_index+1));
	use_EPD_type_index=EPD_270;
}

// Read image data from SRAM
// @param memory_address start address of memory to read
// @param target_buffer the buffer of read data
// @param byte_length the total length to read
void read_SRAM_handle(UINT memory_address,UINT8 *target_buffer,UINT8 byte_length)
{
	SpiRAMReadSeq(memory_address,target_buffer,byte_length);
}

// brief Show image from memory by global update
// param EPD_type_index The defined EPD size
// param previous_image_address The address of memory that stores previous image
// param new_image_address The address of memory that stores new image
// param On_EPD_read_memory External function to read memory
void EpdGlobalUpdate()
{
	cout << "previous_image_address:" << previous_image_address << " - " <<  new_image_address << endl;
	EpdSetPower(true);						// Power on COG Driver
	EpdInitializeDriver(2);					// Initialize COG Driver
	EPD_image_data_globa_handle(previous_image_address,new_image_address,read_SRAM_handle);	// Display image data on EPD from SRAM memory
//	EpdSetPower(false);
	EpdStoreScreen();
}

// EPD partial update function
void EpdPartialUpdate()
{
	cout << "previous_image_address:" << previous_image_address << " - " <<  new_image_address << endl;
	EpdDisplayPartial(use_EPD_type_index,previous_image_address,new_image_address,read_SRAM_handle);
	EpdStoreScreen();
}

// brief Show image from memory by partial update
// The EPD_init, power on and initialize COG just run at the beginning of partial
// update cycle, not per pattern.
// The difference of Partial Update from Global Update is if there is next
// pattern to be updated on EPD, user is able to write next image data to
// memory and COG without power off the COG for better visual experience and
// faster update time.
// The timing to use EPD_power_off for partial update is by use case and ghosting
// improvement.
// param EPD_type_index The defined EPD size
// param previous_image_address The address of memory that stores previous image
// param new_image_address The address of memory that stores new image
// param On_EPD_read_memory External function to read memory
void EpdDisplayPartial(UINT8 EPD_type_index,UINT previous_image_address,UINT new_image_address,EPD_read_memory_handler On_EPD_read_memory)
{
	EPD_image_data_partial_handle(previous_image_address,new_image_address,On_EPD_read_memory);
}

// Store the last updated image to Previous image
void EpdStoreScreen()
{
	UINT32 i;
	UINT8 ImgData[32];
	// SpiRAM_Init();
	for(i=0;i<epd_page_size();i++)
	{
		SpiRAMReadPage((i*32),ImgData);
		SpiRAMWritePage((i*32)+previous_image_address,ImgData);
	}
}

static COG_line_data_packet_type COG_Line;
static UINT8  *data_line_even;
static UINT8  *data_line_odd;
static UINT8  *data_line_scan;

UINT8 EpdInitializeDriver(UINT8 EPD_type_index)
{
	UINT8 SendBuffer[2];
	UINT16 k;

	use_EPD_type_index=EPD_type_index;
	// Empty the Line buffer
	for(k=0;k<=LINE_BUFFER_DATA_SIZE;k++)
		COG_Line.uint8[k]=0x00;
	// Determine the EPD size for driving COG
	data_line_even = &COG_Line.line_data_by_size.line_data_for_270.even[0];
	data_line_odd  = &COG_Line.line_data_by_size.line_data_for_270.odd[0];
	data_line_scan = &COG_Line.line_data_by_size.line_data_for_270.scan[0];

	EpdSpiSend(0x01,(UINT8 *)&COG_parameters[EPD_type_index].channel_select,8);		// Channel select
	EpdSpiSendByte(0x06,0xFF);														// DC/DC frequency setting
	EpdSpiSendByte(0x07,0x9D);														// High power mode OSC setting
	EpdSpiSendByte(0x08,0x00);														// Disable ADC

	// Set Vcom level
	SendBuffer[0]=0xD0;
	SendBuffer[1]=0x00;
	EpdSpiSend(0x09,SendBuffer,2);

	EpdSpiSendByte(0x04,COG_parameters[EPD_type_index].voltage_level);				// Gate and source voltage level
	usleep(5000);
	EpdSpiSendByte(0x03,0x01);														// Driver latch on (cancel register noise)
	EpdSpiSendByte(0x03,0x00);														// Driver latch off
	EpdSpiSendByte(0x05,0x01);														// Start charge pump positive V VGH & VDH on
	SetPWM(5000,2500,0);
	EpdSpiSendByte(0x05,0x03);														// Start charge pump neg voltage VGL & VDL on
	usleep(30000);
	EpdSpiSendByte(0x05,0x0F);														// Set charge pump Vcom_Driver to ON
	usleep(30000);
	EpdSpiSendByte(0x02,0x24);														// Output enable to disable
	SetPWM(5000,2500,0);

	return 1;
}

// brief Write image data from memory to EPD by global update
// note
// Mark from (x0,y0) to (x1,y1) as update area to change data
// Default use whole area of EPD as update area currently
// param previous_image_memory_address The previous image address of memory
// param new_image_memory_address The new image address of memory
// param On_EPD_read_memory Developer needs to create an external function to read memory
void EPD_image_data_globa_handle(UINT previous_image_memory_address,UINT new_image_memory_address,EPD_read_memory_handler On_EPD_read_memory)
{
	_On_EPD_read_handle=On_EPD_read_memory;

	/* Standard 4 stages driving, update from (0,0) to (horizontal_size*8,vertical_size)  */
	EpdStageGlobalHandle(0,COG_parameters[use_EPD_type_index].horizontal_size*8,0,COG_parameters[use_EPD_type_index].vertical_size,previous_image_memory_address,Stage1);
	EpdStageGlobalHandle(0,COG_parameters[use_EPD_type_index].horizontal_size*8,0,COG_parameters[use_EPD_type_index].vertical_size,previous_image_memory_address,Stage2);
	EpdStageGlobalHandle(0,COG_parameters[use_EPD_type_index].horizontal_size*8,0,COG_parameters[use_EPD_type_index].vertical_size,new_image_memory_address,Stage3);
	EpdStageGlobalHandle(0,COG_parameters[use_EPD_type_index].horizontal_size*8,0,COG_parameters[use_EPD_type_index].vertical_size,new_image_memory_address,Stage4);
}

// brief Write image data from memory to EPD by partial update
// Mark from (x0,y0) to (x1,y1) as update area to change data
// Default use whole area of EPD as update area currently
// param previous_image_memory_address The previous image address of memory
// param new_image_memory_address The new image address of memory
// param On_EPD_read_memory Developer needs to create an external function to read memory
void EPD_image_data_partial_handle(UINT previous_image_memory_address,UINT new_image_memory_address,EPD_read_memory_handler On_EPD_read_memory)
{
	_On_EPD_read_handle=On_EPD_read_memory;

	/* Standard 4 stages driving, update from (0,0) to (horizontal_size*8,vertical_size)  */
	epd_stage_partial_handle(0,COG_parameters[use_EPD_type_index].horizontal_size*8,0,COG_parameters[use_EPD_type_index].vertical_size/2,previous_image_memory_address,new_image_memory_address);
//	epd_stage_partial_handle(0,128,0,128,previous_image_memory_address,new_image_memory_address);
	EpdNothingFrame();
	EpdDummyLine(use_EPD_type_index);
}

// brief Get each frame data of stage for global update
// Mark from (x0,y0) to (x1,y1) as update area to change data
// Default use whole area of EPD as update area currently
// @param x0 (x0,y0) as the left/top coordinates
// @param x1 (x1,y1) as the right/bottom coordinates
// @param y0 (x0,y0) as the left/top coordinates
// @param y1 (x1,y1) as the right/bottom coordinates
// @param image_data_address The memory address of image data
// @param stage_no The assigned stage number that will proceed
void EpdStageGlobalHandle(UINT16 x0,UINT16 x1,UINT16 y0,UINT16 y1,UINT image_data_address,UINT8 stage_no)
{
	//	current_frame_time=COG_parameters[use_EPD_type_index].frame_time_offset;
	//	/* Start a system SysTick timer to ensure the same duration of each stage  */
	//	start_EPD_timer();

	//	/* Do while total time of frames exceed stage time
	//	* Per frame */
	//	do
	//	{
	//		epd_frame_global_handle(x0,x1,y0,y1,image_data_address,stage_no);
	//		/* Count the frame time with offset */
	//		current_frame_time=(UINT16)get_current_time_tick()+COG_parameters[use_EPD_type_index].frame_time_offset;
	//	} while (stage_time>current_frame_time);

	//	/* Stop system timer */
	//	stop_EPD_timer();
	epd_frame_global_handle(x0,x1,y0,y1,image_data_address,stage_no);
}

// Get each frame data of stage for partial update
//Mark from (x0,y0) to (x1,y1) as update area to change data
// Default use whole area of EPD as update area currently
// param x0 (x0,y0) as the left/top coordinates
// param x1 (x1,y1) as the right/bottom coordinates
// param y0 (x0,y0) as the left/top coordinates
// param y1 (x1,y1) as the right/bottom coordinates
// param previous_image_data_address The memory address of previous image
// param new_image_data_address The memory address of new image
void epd_stage_partial_handle(UINT16 x0,UINT16 x1,UINT16 y0,UINT16 y1,UINT previous_image_data_address,UINT new_image_data_address)
{
	//	current_frame_time=COG_parameters[use_EPD_type_index].frame_time_offset;

	//	/* Start a system SysTick timer to ensure the same duration of each stage  */
	//	start_EPD_timer();
	//	stage_time=partial_offset_time;
	//	cnt=0;
	//	// Do while total time of frames exceed stage time * Per frame
	//	do
	//	{
	//		//image_data_address=original_image_address;
	//		epd_frame_partial_handle(x0,x1,y0,y1,previous_image_data_address,new_image_data_address);
	//		cnt++;
	//		// Count the frame time with offset
	//		current_frame_time=(UINT16)get_current_time_tick()+COG_parameters[use_EPD_type_index].frame_time_offset;
	//	} while (stage_time>current_frame_time);

	//	/* Stop system timer */
	//	stop_EPD_timer();
	epd_frame_partial_handle(x0,x1,y0,y1,previous_image_data_address,new_image_data_address);	// image_data_address=original_image_address;
}

// brief Get each line data of frame
// Mark from (x0,y0) to (x1,y1) as update area to change data
// Default use whole area of EPD as update area currently
// param x0 (x0,y0) as the left/top coordinates
// param x1 (x1,y1) as the right/bottom coordinates
// param y0 (x0,y0) as the left/top coordinates
// param y1 (x1,y1) as the right/bottom coordinates
// param image_data_address The memory address of image data
// param stage_no The assigned stage number that will proceed
void epd_frame_global_handle(UINT16 x0,UINT16 x1,UINT16 y0,UINT16 y1,UINT image_data_address,UINT8 stage_no )
{
	UINT16 i;
	UINT8 line_array[LINE_BUFFER_DATA_SIZE];

	for(i=0;i<COG_parameters[use_EPD_type_index].vertical_size;i++)
	{
		EpdSpiSendByte(0x04, COG_parameters[use_EPD_type_index].voltage_level);		// Set charge pump voltage level reduce voltage shift

		// Read line data from external array
		if(_On_EPD_read_handle!=NULL)
			_On_EPD_read_handle(image_data_address,line_array,COG_parameters[use_EPD_type_index].horizontal_size);

		/* Get line data */
		if(y0<=i && i<y1)
			epd_line_data_global_handle(x0,x1,line_array,stage_no);
		else
			epd_display_line_dummy_handle();    //last line, set to Nothing frame

		image_data_address+=COG_parameters[use_EPD_type_index].horizontal_size;//LINE_SIZE

		/* Scan byte shift per data line */
		data_line_scan[(i>>2)]= SCAN_TABLE[(i%4)];

		/* For 1.44 inch EPD, the border uses the internal signal control byte. */
		if(use_EPD_type_index==EPD_144)
			COG_Line.line_data_by_size.line_data_for_144.border_byte=0x00;

		EpdSpiSend(0x0A,(UINT8 *)&COG_Line.uint8,COG_parameters[use_EPD_type_index].data_line_size);		// Sending data
		EpdSpiSendByte(0x02,0x2F);		// Turn on Output Enable
		data_line_scan[(i>>2)]=0;
	}
}

// brief The driving stages for getting Odd/Even data per line for global update
// note
// There are 4 stages to complete an image global update on EPD.
// Each of the 4 stages time should be the same uses the same number of frames.
// One dot/pixel is comprised of 2 bits which are White(10), Black(11) or Nothing(01).
// The image data bytes must be divided into Odd and Even bytes.
// The COG driver uses a buffer to write one line of data (FIFO) - interlaced
// Even byte {D(200,y),D(198,y), D(196,y), D(194,y)}, ... ,{D(8,y),D(6,y),D(4,y), D(2,y)}
// Scan byte {S(1), S(2)...}, Odd{D(1,y),D(3,y)...}
// Odd byte  {D(1,y),D(3,y), D(5,y), D(7,y)}, ... ,{D(193,y),D(195,y),D(197,y), D(199,y)}
// One data bit can be
// For more details on the driving stages, please refer to the COG document Section 5.
// param x0 the beginning position of a line
// param x1 the end position of a line
// param line_array The pointer of line array that stores line data
// param stage_no The assigned stage number that will proceed
void epd_line_data_global_handle(UINT16 x0,UINT16 x1,UINT8 *line_array,UINT8 stage_no)
{
	UINT16 i,j;
	UINT8 temp_byte;

	j=COG_parameters[use_EPD_type_index].horizontal_size-1;
	x0 >>= 3;
	x1 = (x1 + 7) >> 3;
	for(i = 0; i < COG_parameters[use_EPD_type_index].horizontal_size; i++)
	{
		if(x0<=i && i<x1)
		{
			temp_byte =line_array[i];
			switch(stage_no)
			{
			case Stage1: // Compensate, Inverse previous image
				data_line_odd[i]     =  ((temp_byte & 0x80) ? BLACK3  : WHITE3);
				data_line_odd[i]    |=  ((temp_byte & 0x20) ? BLACK2  : WHITE2);
				data_line_odd[i]    |=  ((temp_byte & 0x08) ? BLACK1  : WHITE1);
				data_line_odd[i]    |=  ((temp_byte & 0x02) ? BLACK0  : WHITE0);

				data_line_even[j]    = ((temp_byte & 0x01) ? BLACK3  : WHITE3);
				data_line_even[j]   |= ((temp_byte & 0x04) ? BLACK2  : WHITE2);
				data_line_even[j]   |= ((temp_byte & 0x10) ? BLACK1  : WHITE1);
				data_line_even[j]   |= ((temp_byte & 0x40) ? BLACK0  : WHITE0);
				break;
			case Stage2: // White
				data_line_odd[i]     =  ((temp_byte & 0x80) ?  WHITE3 : NOTHING3);
				data_line_odd[i]    |=  ((temp_byte & 0x20) ?  WHITE2 : NOTHING2);
				data_line_odd[i]    |=  ((temp_byte & 0x08) ?  WHITE1 : NOTHING1);
				data_line_odd[i]    |=  ((temp_byte & 0x02) ?  WHITE0 : NOTHING0);

				data_line_even[j]    =  ((temp_byte & 0x01) ?  WHITE3 : NOTHING3);
				data_line_even[j]   |=  ((temp_byte & 0x04) ?  WHITE2 : NOTHING2);
				data_line_even[j]   |=  ((temp_byte & 0x10) ?  WHITE1 : NOTHING1);
				data_line_even[j]   |=  ((temp_byte & 0x40) ?  WHITE0 : NOTHING0);
				break;
			case Stage3: // Inverse new image
				data_line_odd[i]     = ((temp_byte & 0x80) ? BLACK3  : NOTHING3);
				data_line_odd[i]    |= ((temp_byte & 0x20) ? BLACK2  : NOTHING2);
				data_line_odd[i]    |= ((temp_byte & 0x08) ? BLACK1  : NOTHING1);
				data_line_odd[i]    |= ((temp_byte & 0x02) ? BLACK0  : NOTHING0);

				data_line_even[j]    = ((temp_byte & 0x01) ? BLACK3  : NOTHING3);
				data_line_even[j]   |= ((temp_byte & 0x04) ? BLACK2  : NOTHING2);
				data_line_even[j]   |= ((temp_byte & 0x10) ? BLACK1  : NOTHING1);
				data_line_even[j]   |= ((temp_byte & 0x40) ? BLACK0  : NOTHING0);
				break;
			case Stage4: // New image
				data_line_odd[i]     = ((temp_byte & 0x80) ? WHITE3  : BLACK3 );
				data_line_odd[i]    |= ((temp_byte & 0x20) ? WHITE2  : BLACK2 );
				data_line_odd[i]    |= ((temp_byte & 0x08) ? WHITE1  : BLACK1 );
				data_line_odd[i]    |= ((temp_byte & 0x02) ? WHITE0  : BLACK0 );

				data_line_even[j]    = ((temp_byte & 0x01) ? WHITE3  : BLACK3 );
				data_line_even[j]   |= ((temp_byte & 0x04) ? WHITE2  : BLACK2 );
				data_line_even[j]   |= ((temp_byte & 0x10) ? WHITE1  : BLACK1 );
				data_line_even[j]   |= ((temp_byte & 0x40) ? WHITE0  : BLACK0 );
				break;
			}
		}
		else
		{
			data_line_odd[i]    =NOTHING;
			data_line_even[j]   =NOTHING;
		}
		j--;
	}
}

// Write Nothing frame
void epd_display_line_dummy_handle(void)
{
	UINT8 i;

	for(i=0;i<COG_parameters[use_EPD_type_index].horizontal_size;i++)
	{
		data_line_odd[i] =NOTHING;
		data_line_even[i]=NOTHING;
	}
}

// brief Write Nothing Frame to COG
void EpdNothingFrame()
{
	UINT16 i;

	for(i=0;i<COG_parameters[use_EPD_type_index].horizontal_size;i++)
	{
		data_line_even[i]=NOTHING;
		data_line_odd[i]=NOTHING;
	}
	for(i=0;i<(COG_parameters[use_EPD_type_index].vertical_size);i++)
	{
		EpdSpiSendByte(0x04, COG_parameters[use_EPD_type_index].voltage_level);							// Set charge pump voltage level reduce voltage shift
		data_line_scan[(i>>2)]=SCAN_TABLE[(i%4)];														// Scan byte shift per data line
		EpdSpiSend(0x0A,(UINT8 *)&COG_Line.uint8, COG_parameters[use_EPD_type_index].data_line_size);	// Sending data
		EpdSpiSendByte(0x02,0x2F);																		// Turn on Output Enable
		data_line_scan[(i>>2)]=0;
	}
}

UINT8 Rotatebyte(UINT8 b)
{
	UINT8 tmp=b & 0xaa;
	if(b & 0x40) tmp|=0x01;
	if(b & 0x10) tmp|=0x04;
	if(b & 0x04) tmp|=0x10;
	if(b & 0x01) tmp|=0x40;
	return tmp;
}

// Send Border dummy line (for 1.44" EPD at Power off COG stage)
// @param EPD_type_index The defined EPD size
void EpdBorderLine(UINT8 EPD_type_index)
{
	UINT16 i;

	for(i=0;i<COG_parameters[EPD_type_index].horizontal_size;i++)
	{
		data_line_even[i]=0x00;//NOTHING;
		data_line_odd[i]=0x00;//NOTHING;
	}
	for(i=0;i<(COG_parameters[EPD_type_index].vertical_size/4);i++)
		data_line_scan[i]=0x00;

	COG_Line.line_data_by_size.line_data_for_144.border_byte=0xAA;
	EpdSpiSendByte(0x04,0x03);
	EpdSpiSend(0x0A,(UINT8 *)&COG_Line.uint8,COG_parameters[EPD_type_index].data_line_size);		// SPI (0x0a, line data....)
	EpdSpiSendByte(0x02,0x2F);																	// SPI (0x02,0x25)
}

// Write Dummy Line to COG
// note A line whose all Scan Bytes are 0x00
// param EPD_type_index The defined EPD size
void EpdDummyLine(UINT8 EPD_type_index)
{
	UINT8 i;
	for(i=0;i<(COG_parameters[EPD_type_index].vertical_size/8);i++)
	{
		switch(EPD_type_index)
		{
		case EPD_144:
			COG_Line.line_data_by_size.line_data_for_144.scan[i]=0x00;
			break;
		case EPD_200:
			COG_Line.line_data_by_size.line_data_for_200.scan[i]=0x00;
			break;
		case EPD_270:
			COG_Line.line_data_by_size.line_data_for_270.scan[i]=0x00;
			break;
		}
	}
	EpdSpiSendByte(0x04, COG_parameters[EPD_type_index].voltage_level);								// Set charge pump voltage level reduce voltage shift
	EpdSpiSend(0x0A,(UINT8 *)&COG_Line.uint8,COG_parameters[EPD_type_index].data_line_size);		// Sending data
	EpdSpiSendByte (0x02,0x2F);																	// Turn on Output Enable
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

int EpdWriteSpi(unsigned char data)
{
	char buf[SPI_MAX_BUF];

	buf[0]=data;
	write(spiEPDFile,buf,1);
	return 1;
}

UINT8 EpdReadSpiRAM()
{
	char buf[SPI_MAX_BUF];
	read(spiEPDFile,buf,1);
	return buf[0];
}

void EpdSpiSend(UINT8 register_index,UINT8 *register_data,UINT8 length)
{
	gpio_set_value(GPIO_EPD_CS,LOW);
	EpdWriteSpi(0x70); // header of Register Index
	EpdWriteSpi(register_index);

	gpio_set_value(GPIO_EPD_CS,HIGH);
	usleep(10);
	gpio_set_value(GPIO_EPD_CS,LOW);

	EpdWriteSpi(0x72); // header of Register Data of write command
	while(length--)
		EpdWriteSpi(*register_data++);
	gpio_set_value(GPIO_EPD_CS,HIGH);
	usleep(10);
}

void EpdSpiSendByte(UINT8 register_index,UINT8 register_data)
{
	gpio_set_value(GPIO_EPD_CS,LOW);
	EpdWriteSpi(0x70); // header of Register Index
	EpdWriteSpi(register_index);
	gpio_set_value(GPIO_EPD_CS,HIGH);
	usleep(10);
	gpio_set_value(GPIO_EPD_CS,LOW);
	EpdWriteSpi(0x72); // header of Register Data
	EpdWriteSpi(register_data);
	gpio_set_value(GPIO_EPD_CS,HIGH);
	usleep(10);
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

void EpdSetPower(bool mode)
{
	if(mode)
	{
		gpio_set_value(GPIO_DISCHARGE,LOW);
		gpio_set_value(GPIO_RESET,LOW);
		//		OpenSpiEPD(&spiEPDFile);
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
		EpdNothingFrame();
		if(use_EPD_type_index==EPD_144)
		{
			gpio_set_value(GPIO_BORDER,HIGH);
			EpdBorderLine(use_EPD_type_index);
			usleep(200000);
		}
		else
		{
			EpdDummyLine(use_EPD_type_index);
			usleep(25000);
			gpio_set_value(GPIO_BORDER,LOW);
			usleep(200000);
			gpio_set_value(GPIO_BORDER,HIGH);
		}
		EpdSpiSendByte(0x03,0x01);		// Latch reset turn on
		EpdSpiSendByte(0x02,0x05);		// Output enable off
		EpdSpiSendByte(0x05,0x0E);		// Power off charge pump Vcom
		EpdSpiSendByte (0x05,0x02);		// Power off charge negative voltage
		EpdSpiSendByte(0x04,0x0C);		// Discharge
		usleep(120000);
		EpdSpiSendByte(0x05,0x00);		// Turn off all charge pumps
		EpdSpiSendByte(0x07,0x0D);		// Turn off osc
		EpdSpiSendByte(0x04,0x50);		// Discharge internal
		usleep(40000);
		EpdSpiSendByte(0x04,0xA0);		// Discharge internal
		usleep(40000);
		EpdSpiSendByte(0x04,0x00);		// Discharge internal

		// Set power and signals = 0
		gpio_set_value(GPIO_RESET,LOW);
		//		spi_detach ();
		gpio_set_value(GPIO_EPD_CS,LOW);
		gpio_set_value(GPIO_RESET,LOW);
		//EPD_border_low();

		gpio_set_value(GPIO_DISCHARGE,HIGH);	// External discharge = 1
		usleep(150000);
		gpio_set_value(GPIO_DISCHARGE,LOW);		// External discharge = 0
	}
}

void EpdCls(unsigned char fillData)
{
	EpdWriteCmd(CMD_SET_COLUMN_ADDRESS);
	EpdWriteData(0x1C);

	EpdWriteCmd(CMD_SET_ROW_ADDRESS);
	EpdWriteData(0x00);

	EpdWriteCmd(CMD_WRITE_RAM_CMD);
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

int EpdWriteCmd(unsigned char cmd)
{
	gpio_set_value(GPIO_BUSY,LOW);
	return WriteSpiEPD(cmd);
}

int EpdWriteData(unsigned char data)
{
	gpio_set_value(GPIO_BUSY,HIGH);
	return WriteSpiEPD(data);
}

// brief Get each line data of frame for partial update
// Mark from (x0,y0) to (x1,y1) as update area to change data
// Default use whole area of EPD as update area currently
// param x0 (x0,y0) as the left/top coordinates
// param x1 (x1,y1) as the right/bottom coordinates
// param y0 (x0,y0) as the left/top coordinates
// param y1 (x1,y1) as the right/bottom coordinates
// param previous_image_data_address The memory address of previous image data
// param new_image_data_address The memory address of new image data
void epd_frame_partial_handle(UINT16 x0,UINT16 x1,UINT16 y0,UINT16 y1,UINT previous_image_data_address,UINT new_image_data_address)
{
	UINT16 i;
	UINT8 previous_line_array[LINE_BUFFER_DATA_SIZE];
	UINT8 new_line_array[LINE_BUFFER_DATA_SIZE];
	for(i=0;i<COG_parameters[use_EPD_type_index].vertical_size; i++)
	{
		EpdSpiSendByte(0x04,COG_parameters[use_EPD_type_index].voltage_level);		// Set charge pump voltage level reduce voltage shift

		//Read line data from external array
		if(_On_EPD_read_handle!=NULL)
		{
			_On_EPD_read_handle(previous_image_data_address,previous_line_array,COG_parameters[use_EPD_type_index].horizontal_size);
			_On_EPD_read_handle(new_image_data_address,new_line_array,COG_parameters[use_EPD_type_index].horizontal_size);
		}
		epd_line_data_partial_handle(x0,x1,previous_line_array,new_line_array);
		previous_image_data_address+=COG_parameters[use_EPD_type_index].horizontal_size;//LINE_SIZE;
		new_image_data_address+=COG_parameters[use_EPD_type_index].horizontal_size;//LINE_SIZE;
		data_line_scan[(i>>2)]= SCAN_TABLE[(i%4)];		// Scan byte shift per data line

		// For 1.44 inch EPD, the border uses the internal signal control byte.
		if(use_EPD_type_index==EPD_144)
			COG_Line.line_data_by_size.line_data_for_144.border_byte=0x00;
		EpdSpiSend(0x0A,(UINT8 *)&COG_Line.uint8,COG_parameters[use_EPD_type_index].data_line_size);	// Sending data
		EpdSpiSendByte(0x02,0x2F);		// Turn on Output Enable
		data_line_scan[(i>>2)]=0;
	}
}

// The driving stages for getting Odd/Even data per line for partial update
// The partial update uses one stage to update EPD.
// If the new data byte is same as previous data byte, send “Nothing” data byte
// which means the data byte on EPD won’t be changed.
// If the new data byte is different from the previous data byte, send the new data byte.
// param x0 the beginning position of a line
// param x1 the end position of a line
// param previous_line_array The pointer of line array that stores previous image
// param new_line_array The pointer of line array that stores new image
void epd_line_data_partial_handle(UINT16 x0,UINT16 x1,UINT8 *previous_line_array,UINT8 *new_line_array)
{
	UINT16 i,j;
	UINT8 draw_byte;
	j=COG_parameters[use_EPD_type_index].horizontal_size-1;
	x0>>=3;
	x1=(x1 + 7)>>3;
	for(i=0;i<COG_parameters[use_EPD_type_index].horizontal_size; i++)
	{
		new_line_array[i]=Rotatebyte(new_line_array[i]);
		previous_line_array[i]=Rotatebyte(previous_line_array[i]);
		draw_byte=new_line_array[i] ^ previous_line_array[i];
		data_line_odd[i]     = (draw_byte & 0xaa) | (((previous_line_array[i] >>1) & 0x55) & ((draw_byte & 0xaa) >>1)) ;
		data_line_even[j--]  = ((draw_byte <<1) & 0xaa) | ((previous_line_array[i] & 0x55) & (((draw_byte <<1) & 0xaa) >>1));
	}
}

void EpdSetPixel(unsigned char x,unsigned char y,unsigned int color)
{
	EpdWriteCmd(CMD_SET_COLUMN_ADDRESS);
	EpdWriteData(0x1C+(x>>2));

	EpdWriteCmd(CMD_SET_ROW_ADDRESS);
	EpdWriteData(y);

	EpdWriteCmd(CMD_WRITE_RAM_CMD);
	EpdWriteData(color>>8);
	EpdWriteData(color);
}

void EpdTest()
{
//	UINT16 i;
//	UINT8 previous_line_array[LINE_BUFFER_DATA_SIZE];
//	UINT8 new_line_array[LINE_BUFFER_DATA_SIZE];
//	for(i=0;i<COG_parameters[use_EPD_type_index].vertical_size; i++)
//	{
//		EpdSpiSendByte(0x04,COG_parameters[use_EPD_type_index].voltage_level);		// Set charge pump voltage level reduce voltage shift

//		//Read line data from external array
//		if(_On_EPD_read_handle!=NULL)
//		{
//			_On_EPD_read_handle(previous_image_data_address,previous_line_array,COG_parameters[use_EPD_type_index].horizontal_size);
//			_On_EPD_read_handle(new_image_data_address,new_line_array,COG_parameters[use_EPD_type_index].horizontal_size);
//		}
//		epd_line_data_partial_handle(x0,x1,previous_line_array,new_line_array);
//		previous_image_data_address+=COG_parameters[use_EPD_type_index].horizontal_size;//LINE_SIZE;
//		new_image_data_address+=COG_parameters[use_EPD_type_index].horizontal_size;//LINE_SIZE;
//		data_line_scan[(i>>2)]= SCAN_TABLE[(i%4)];		// Scan byte shift per data line

//		// For 1.44 inch EPD, the border uses the internal signal control byte.
//		if(use_EPD_type_index==EPD_144)
//			COG_Line.line_data_by_size.line_data_for_144.border_byte=0x00;
//		EpdSpiSend(0x0A,(UINT8 *)&COG_Line.uint8,COG_parameters[use_EPD_type_index].data_line_size);	// Sending data
//		EpdSpiSendByte(0x02,0x2F);		// Turn on Output Enable
//		data_line_scan[(i>>2)]=0;
//	}

//	UINT16 i;
//	UINT8 tab[256];
//	char str[256];

//	for(int i=0;i<33;i++)
//		tab[i]=0xFF;
////	tab[0]=0xFF;

//	for(i=0;i<COG_parameters[use_EPD_type_index].horizontal_size;i++)
//	{
//		data_line_even[i]=0xFF;
//		data_line_odd[i]=0xFF;
//	}
////	data_line_even[0]=0x01;
//	for(i=0;i<(COG_parameters[use_EPD_type_index].vertical_size/4);i++)
//		data_line_scan[i]=0x00;

//	sprintf(str,"data_line_size=%d\r\n",COG_parameters[use_EPD_type_index].data_line_size);
//	cout << str;

//	EpdSpiSendByte(0x04,0x00);		// Set charge pump voltage level reduce voltage shift
////	EpdSpiSend(0x0A,(UINT8 *)&COG_Line.uint8,COG_parameters[use_EPD_type_index].data_line_size);	// Sending data
//	EpdSpiSend(0x0A,(UINT8 *)&tab,33);	// Sending data
//	EpdSpiSendByte(0x02,0x2F);		// Turn on Output Enable

//	EpdSpiSendByte(0x04,0x00);		// Set charge pump voltage level reduce voltage shift
////	EpdSpiSend(0x0A,(UINT8 *)&COG_Line.uint8,COG_parameters[use_EPD_type_index].data_line_size);	// Sending data
//	EpdSpiSend(0x0A,(UINT8 *)&tab,33);	// Sending data
//	EpdSpiSendByte(0x02,0x2F);		// Turn on Output Enable

//	EpdNothingFrame();
//	EpdDummyLine(use_EPD_type_index);
}
