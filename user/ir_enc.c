/******************************************************************************
 * Copyright 2013-2015 Espressif Systems
 *           2015 <>< Charles Lohr
 *
 *  See ws2812_i2s.c for more information.
 */


#include "slc_register.h"
#include "esp82xxutil.h"
#include <c_types.h>
#include "ir_enc.h"
#include "user_interface.h"
#include "pin_mux_register.h"

#define WS_I2S_BCK 36  //Shooting for 37 kHz.
#define WS_I2S_DIV 60

#ifndef i2c_bbpll
#define i2c_bbpll                                 0x67
#define i2c_bbpll_en_audio_clock_out            4
#define i2c_bbpll_en_audio_clock_out_msb        7
#define i2c_bbpll_en_audio_clock_out_lsb        7
#define i2c_bbpll_hostid                           4

#define i2c_writeReg_Mask(block, host_id, reg_add, Msb, Lsb, indata)  rom_i2c_writeReg_Mask(block, host_id, reg_add, Msb, Lsb, indata)
#define i2c_readReg_Mask(block, host_id, reg_add, Msb, Lsb)  rom_i2c_readReg_Mask(block, host_id, reg_add, Msb, Lsb)
#define i2c_writeReg_Mask_def(block, reg_add, indata) \
      i2c_writeReg_Mask(block, block##_hostid,  reg_add,  reg_add##_msb,  reg_add##_lsb,  indata)
#define i2c_readReg_Mask_def(block, reg_add) \
      i2c_readReg_Mask(block, block##_hostid,  reg_add,  reg_add##_msb,  reg_add##_lsb)
#endif
#ifndef ETS_SLC_INUM
#define ETS_SLC_INUM       1
#endif



//From i2s_reg.h
#define DR_REG_I2S_BASE (0x60000e00)

#define I2STXFIFO  (DR_REG_I2S_BASE + 0x0000)
#define I2SRXFIFO  (DR_REG_I2S_BASE + 0x0004)
#define I2SCONF  (DR_REG_I2S_BASE + 0x0008)
#define I2S_BCK_DIV_NUM 0x0000003F
#define I2S_BCK_DIV_NUM_S 22
#define I2S_CLKM_DIV_NUM 0x0000003F
#define I2S_CLKM_DIV_NUM_S 16
#define I2S_BITS_MOD 0x0000000F
#define I2S_BITS_MOD_S 12
#define I2S_RECE_MSB_SHIFT (BIT(11))
#define I2S_TRANS_MSB_SHIFT (BIT(10))
#define I2S_I2S_RX_START (BIT(9))
#define I2S_I2S_TX_START (BIT(8))
#define I2S_MSB_RIGHT (BIT(7))
#define I2S_RIGHT_FIRST (BIT(6))
#define I2S_RECE_SLAVE_MOD (BIT(5))
#define I2S_TRANS_SLAVE_MOD (BIT(4))
#define I2S_I2S_RX_FIFO_RESET (BIT(3))
#define I2S_I2S_TX_FIFO_RESET (BIT(2))
#define I2S_I2S_RX_RESET (BIT(1))
#define I2S_I2S_TX_RESET (BIT(0))
#define I2S_I2S_RESET_MASK 0xf

 #define I2SINT_RAW (DR_REG_I2S_BASE + 0x000c)
#define I2S_I2S_TX_REMPTY_INT_RAW (BIT(5))
#define I2S_I2S_TX_WFULL_INT_RAW (BIT(4))
#define I2S_I2S_RX_REMPTY_INT_RAW (BIT(3))
#define I2S_I2S_RX_WFULL_INT_RAW (BIT(2))
#define I2S_I2S_TX_PUT_DATA_INT_RAW (BIT(1))
#define I2S_I2S_RX_TAKE_DATA_INT_RAW (BIT(0))


#define I2SINT_ST (DR_REG_I2S_BASE + 0x0010)
#define I2S_I2S_TX_REMPTY_INT_ST (BIT(5))
#define I2S_I2S_TX_WFULL_INT_ST (BIT(4))
#define I2S_I2S_RX_REMPTY_INT_ST (BIT(3))
#define I2S_I2S_RX_WFULL_INT_ST (BIT(2))
#define I2S_I2S_TX_PUT_DATA_INT_ST (BIT(1))
#define I2S_I2S_RX_TAKE_DATA_INT_ST (BIT(0))

 #define I2SINT_ENA (DR_REG_I2S_BASE + 0x0014)
#define I2S_I2S_TX_REMPTY_INT_ENA (BIT(5))
#define I2S_I2S_TX_WFULL_INT_ENA (BIT(4))
#define I2S_I2S_RX_REMPTY_INT_ENA (BIT(3))
#define I2S_I2S_RX_WFULL_INT_ENA (BIT(2))
#define I2S_I2S_TX_PUT_DATA_INT_ENA (BIT(1))
#define I2S_I2S_RX_TAKE_DATA_INT_ENA (BIT(0))

 #define I2SINT_CLR (DR_REG_I2S_BASE + 0x0018)
#define I2S_I2S_TX_REMPTY_INT_CLR (BIT(5))
#define I2S_I2S_TX_WFULL_INT_CLR (BIT(4))
#define I2S_I2S_RX_REMPTY_INT_CLR (BIT(3))
#define I2S_I2S_RX_WFULL_INT_CLR (BIT(2))
#define I2S_I2S_PUT_DATA_INT_CLR (BIT(1))
#define I2S_I2S_TAKE_DATA_INT_CLR (BIT(0))

#define I2STIMING (DR_REG_I2S_BASE + 0x001c)
#define I2S_TRANS_BCK_IN_INV (BIT(22))
#define I2S_RECE_DSYNC_SW (BIT(21))
#define I2S_TRANS_DSYNC_SW (BIT(20))
#define I2S_RECE_BCK_OUT_DELAY 0x00000003
#define I2S_RECE_BCK_OUT_DELAY_S 18
#define I2S_RECE_WS_OUT_DELAY 0x00000003
#define I2S_RECE_WS_OUT_DELAY_S 16
#define I2S_TRANS_SD_OUT_DELAY 0x00000003
#define I2S_TRANS_SD_OUT_DELAY_S 14
#define I2S_TRANS_WS_OUT_DELAY 0x00000003
#define I2S_TRANS_WS_OUT_DELAY_S 12
#define I2S_TRANS_BCK_OUT_DELAY 0x00000003
#define I2S_TRANS_BCK_OUT_DELAY_S 10
#define I2S_RECE_SD_IN_DELAY 0x00000003
#define I2S_RECE_SD_IN_DELAY_S 8
#define I2S_RECE_WS_IN_DELAY 0x00000003
#define I2S_RECE_WS_IN_DELAY_S 6
#define I2S_RECE_BCK_IN_DELAY 0x00000003
#define I2S_RECE_BCK_IN_DELAY_S 4
#define I2S_TRANS_WS_IN_DELAY 0x00000003
#define I2S_TRANS_WS_IN_DELAY_S 2
#define I2S_TRANS_BCK_IN_DELAY 0x00000003
#define I2S_TRANS_BCK_IN_DELAY_S 0

#define I2S_FIFO_CONF (DR_REG_I2S_BASE + 0x0020)
#define I2S_I2S_RX_FIFO_MOD 0x00000007
#define I2S_I2S_RX_FIFO_MOD_S 16
#define I2S_I2S_TX_FIFO_MOD 0x00000007
#define I2S_I2S_TX_FIFO_MOD_S 13
#define I2S_I2S_DSCR_EN (BIT(12))
#define I2S_I2S_TX_DATA_NUM 0x0000003F
#define I2S_I2S_TX_DATA_NUM_S 6
#define I2S_I2S_RX_DATA_NUM 0x0000003F
#define I2S_I2S_RX_DATA_NUM_S 0


#define I2SRXEOF_NUM (DR_REG_I2S_BASE + 0x0024)
#define I2S_I2S_RX_EOF_NUM 0xFFFFFFFF
#define I2S_I2S_RX_EOF_NUM_S 0

#define I2SCONF_SIGLE_DATA (DR_REG_I2S_BASE + 0x0028)
#define I2S_I2S_SIGLE_DATA 0xFFFFFFFF
#define I2S_I2S_SIGLE_DATA_S 0

#define I2SCONF_CHAN (DR_REG_I2S_BASE + 0x002c)
#define I2S_RX_CHAN_MOD 0x00000003
#define I2S_RX_CHAN_MOD_S 3
#define I2S_TX_CHAN_MOD 0x00000007
#define I2S_TX_CHAN_MOD_S 0


//From sdio_slv.h


struct sdio_queue
{
	uint32	blocksize:12;
	uint32	datalen:12;
	uint32	unused:5;
	uint32	sub_sof:1;
	uint32 	eof:1;
	uint32	owner:1;

	uint32	buf_ptr;
	uint32	next_link_ptr;
};

struct sdio_slave_status_element
{
	uint32 wr_busy:1;
	uint32 rd_empty :1;
	uint32 comm_cnt :3;
	uint32 intr_no :3;
	uint32 rx_length:16;
	uint32 res:8;
};

union sdio_slave_status
{
	struct sdio_slave_status_element elm_value;
	uint32 word_value;
};

#define RX_AVAILIBLE 2
#define TX_AVAILIBLE 1
#define INIT_STAGE	 0

#define SDIO_QUEUE_LEN 8
#define MOSI  0
#define MISO  1
#define SDIO_DATA_ERROR 6

#define SLC_INTEREST_EVENT (SLC_TX_EOF_INT_ENA | SLC_RX_EOF_INT_ENA | SLC_RX_UDF_INT_ENA | SLC_TX_DSCR_ERR_INT_ENA)
#define TRIG_TOHOST_INT()	SET_PERI_REG_MASK(SLC_INTVEC_TOHOST , BIT0);\
							CLEAR_PERI_REG_MASK(SLC_INTVEC_TOHOST , BIT0)


///Rest of program...

//Pointer to the I2S DMA buffer data
//static unsigned int i2sBuf[I2SDMABUFCNT][I2SDMABUFLEN];
//I2S DMA buffer descriptors
//static struct sdio_queue i2sBufDesc[I2SDMABUFCNT];
static struct sdio_queue i2sBufDescOut;
static struct sdio_queue i2sBufDescZeroes;

static unsigned int i2sZeroes[32];
static unsigned int i2sBlock[WS_BLOCKSIZE/4];

//Queue which contains empty DMA buffers
//DMA underrun counter

LOCAL void slc_isr(void) {
	struct sdio_queue *finishedDesc;
	uint32 slc_intr_status;
	int x;

	slc_intr_status = READ_PERI_REG(SLC_INT_STATUS);
	//clear all intr flags
	WRITE_PERI_REG(SLC_INT_CLR, 0xffffffff);//slc_intr_status);
	if ( (slc_intr_status & SLC_RX_EOF_INT_ST))
	{
		i2sBufDescZeroes.next_link_ptr=(uint32_t)&i2sBufDescZeroes;
		//An interrupt got fired.  It was probably for the zero.
		//Either way, put zero back at itself from the data.
	}
}



//Initialize I2S subsystem for DMA circular buffer use
void ICACHE_FLASH_ATTR ir_init()
{
	int x, y;
	
	//Reset DMA
	SET_PERI_REG_MASK(SLC_CONF0, SLC_RXLINK_RST);//|SLC_TXLINK_RST);
	CLEAR_PERI_REG_MASK(SLC_CONF0, SLC_RXLINK_RST);//|SLC_TXLINK_RST);

	//Clear DMA int flags
	SET_PERI_REG_MASK(SLC_INT_CLR,  0xffffffff);
	CLEAR_PERI_REG_MASK(SLC_INT_CLR,  0xffffffff);

	//Enable and configure DMA
	CLEAR_PERI_REG_MASK(SLC_CONF0, (SLC_MODE<<SLC_MODE_S));
	SET_PERI_REG_MASK(SLC_CONF0,(1<<SLC_MODE_S));
	SET_PERI_REG_MASK(SLC_RX_DSCR_CONF,SLC_INFOR_NO_REPLACE|SLC_TOKEN_NO_REPLACE);
	CLEAR_PERI_REG_MASK(SLC_RX_DSCR_CONF, SLC_RX_FILL_EN|SLC_RX_EOF_MODE | SLC_RX_FILL_MODE);

	i2sBufDescOut.owner = 1;
	i2sBufDescOut.eof = 1;
	i2sBufDescOut.sub_sof = 0;
	i2sBufDescOut.datalen = WS_BLOCKSIZE;  //Size (in bytes)
	i2sBufDescOut.blocksize = WS_BLOCKSIZE; //Size (in bytes)
	i2sBufDescOut.buf_ptr=(uint32_t)&i2sBlock[0];
	i2sBufDescOut.unused=0;
	i2sBufDescOut.next_link_ptr=(uint32_t)&i2sBufDescZeroes; //At the end, just redirect the DMA to the zero buffer.

	i2sBufDescZeroes.owner = 1;
	i2sBufDescZeroes.eof = 1;
	i2sBufDescZeroes.sub_sof = 0;
	i2sBufDescZeroes.datalen = 32;
	i2sBufDescZeroes.blocksize = 32;
	i2sBufDescZeroes.buf_ptr=(uint32_t)&i2sZeroes[0];
	i2sBufDescZeroes.unused=0;
	i2sBufDescZeroes.next_link_ptr=(uint32_t)&i2sBufDescZeroes;


	for( x = 0; x < 32; x++ )
	{
		i2sZeroes[x] = 0x00;
	}
	for( x = 0; x < WS_BLOCKSIZE/4; x++ )
	{
		i2sBlock[x] = 0x00000000;//(x == 0 || x == 999)?0xaa:0x00;
	}


//	CLEAR_PERI_REG_MASK(SLC_TX_LINK,SLC_TXLINK_DESCADDR_MASK);
//	SET_PERI_REG_MASK(SLC_TX_LINK, ((uint32)&i2sBufDescZeroes) & SLC_TXLINK_DESCADDR_MASK); //any random desc is OK, we don't use TX but it needs something valid

	CLEAR_PERI_REG_MASK(SLC_RX_LINK,SLC_RXLINK_DESCADDR_MASK);
	SET_PERI_REG_MASK(SLC_RX_LINK, ((uint32)&i2sBufDescOut) & SLC_RXLINK_DESCADDR_MASK);


	//Attach the DMA interrupt
	ets_isr_attach(ETS_SLC_INUM, slc_isr);
	//Enable DMA operation intr
	WRITE_PERI_REG(SLC_INT_ENA,  SLC_RX_EOF_INT_ENA);
	//clear any interrupt flags that are set
	WRITE_PERI_REG(SLC_INT_CLR, 0xffffffff);
	///enable DMA intr in cpu
	ets_isr_unmask(1<<ETS_SLC_INUM);


	//Start transmission
//	SET_PERI_REG_MASK(SLC_TX_LINK, SLC_TXLINK_START);
	SET_PERI_REG_MASK(SLC_RX_LINK, SLC_RXLINK_START);

//----

	//Init pins to i2s functions
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_I2SO_DATA);
//	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_I2SO_WS);
//	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_I2SO_BCK);

	//Enable clock to i2s subsystem
	i2c_writeReg_Mask_def(i2c_bbpll, i2c_bbpll_en_audio_clock_out, 1);

	//Reset I2S subsystem
	CLEAR_PERI_REG_MASK(I2SCONF,I2S_I2S_RESET_MASK);
	SET_PERI_REG_MASK(I2SCONF,I2S_I2S_RESET_MASK);
	CLEAR_PERI_REG_MASK(I2SCONF,I2S_I2S_RESET_MASK);

	//Select 16bits per channel (FIFO_MOD=0), no DMA access (FIFO only)
	CLEAR_PERI_REG_MASK(I2S_FIFO_CONF, I2S_I2S_DSCR_EN|(I2S_I2S_RX_FIFO_MOD<<I2S_I2S_RX_FIFO_MOD_S)|(I2S_I2S_TX_FIFO_MOD<<I2S_I2S_TX_FIFO_MOD_S));
	//Enable DMA in i2s subsystem
	SET_PERI_REG_MASK(I2S_FIFO_CONF, I2S_I2S_DSCR_EN);

	//tx/rx binaureal
//	CLEAR_PERI_REG_MASK(I2SCONF_CHAN, (I2S_TX_CHAN_MOD<<I2S_TX_CHAN_MOD_S)|(I2S_RX_CHAN_MOD<<I2S_RX_CHAN_MOD_S));

	//trans master&rece slave,MSB shift,right_first,msb right


	//Clear int
	SET_PERI_REG_MASK(I2SINT_CLR,  
			I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);
	CLEAR_PERI_REG_MASK(I2SINT_CLR, 
			I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);


	CLEAR_PERI_REG_MASK(I2SCONF, I2S_TRANS_SLAVE_MOD|
						(I2S_BITS_MOD<<I2S_BITS_MOD_S)|
						(I2S_BCK_DIV_NUM <<I2S_BCK_DIV_NUM_S)|
						(I2S_CLKM_DIV_NUM<<I2S_CLKM_DIV_NUM_S));
	SET_PERI_REG_MASK(I2SCONF, I2S_RIGHT_FIRST|I2S_MSB_RIGHT|I2S_RECE_SLAVE_MOD|
						I2S_RECE_MSB_SHIFT|I2S_TRANS_MSB_SHIFT|
						(((WS_I2S_BCK)&I2S_BCK_DIV_NUM )<<I2S_BCK_DIV_NUM_S)|
						(((WS_I2S_DIV)&I2S_CLKM_DIV_NUM)<<I2S_CLKM_DIV_NUM_S));


	//No idea if ints are needed...
	//clear int
	SET_PERI_REG_MASK(I2SINT_CLR, I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);
	CLEAR_PERI_REG_MASK(I2SINT_CLR, I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);
	//enable int
	SET_PERI_REG_MASK(I2SINT_ENA,  I2S_I2S_RX_REMPTY_INT_ENA|I2S_I2S_RX_TAKE_DATA_INT_ENA);


	//Start transmission
	SET_PERI_REG_MASK(I2SCONF,I2S_I2S_TX_START);
}



void ICACHE_FLASH_ATTR ir_push( uint8_t * buffer, uint16_t buffersize )
{
	uint16_t place;
	int i;
	uint16_t * bufferpl_init = (uint16_t*)&i2sBlock[0];
	uint16_t * bufferpl_end = bufferpl_init;

	for( i = 0; i < 42; i++ )
	{
		*(bufferpl_end++) = 0xAAAA; //Alternating 1's and 0's.
	}

	for( i = 0; i < 20; i++ )  //Off for a bit.
	{
		*(bufferpl_end++) = 0;
	}

	for( i = 0; i < buffersize; i++ )
	{
		int mask = 0x80;
		uint8_t tosend = *(buffer++);
		for( ; mask; mask>>=1 )
		{
			*(bufferpl_end++) = 0xAAAA; //Alternating 1's and 0's.
			*(bufferpl_end++) = 0xAAAA; //Alternating 1's and 0's.
			*(bufferpl_end++) = 0x0000;
			*(bufferpl_end++) = 0xAAAA; //Alternating 1's and 0's.
			*(bufferpl_end++) = 0x0000;
			*(bufferpl_end++) = 0x0000;
			if( tosend & mask )
			{
				*(bufferpl_end++) = 0x0000;
				*(bufferpl_end++) = 0x0000;
				*(bufferpl_end++) = 0x0000;
				*(bufferpl_end++) = 0x0000;
				*(bufferpl_end++) = 0x0000;
				*(bufferpl_end++) = 0x0000;
			}
		}
	}

	//Send ending pulse.
	*(bufferpl_end++) = 0xAAAA;
	*(bufferpl_end++) = 0xAAAA;
	*(bufferpl_end++) = 0x0000;
	*(bufferpl_end++) = 0xAAAA;

	*(bufferpl_end++) = 0x0000;
	*(bufferpl_end++) = 0x0000;

	uint16_t sending = bufferpl_end - bufferpl_init;
	sending &= ~1;  //Make sure it rounds down to a quantity-of-4 bytes.

	printf( "SENDING: %d\n", sending );
	//int sending = 32;

	i2sBufDescOut.owner = 1;
	i2sBufDescOut.eof = 1;
	i2sBufDescOut.sub_sof = 0;
	i2sBufDescOut.datalen = sending*2;  //Size (in bytes)
	i2sBufDescOut.blocksize = sending*2; //Size (in bytes)
	i2sBufDescOut.buf_ptr = (uint32_t)&i2sBlock[0];
	i2sBufDescOut.unused = 0;

	i2sBufDescZeroes.next_link_ptr=(uint32_t)&i2sBufDescOut; //At the end, just redirect the DMA to the zero buffer.


}



