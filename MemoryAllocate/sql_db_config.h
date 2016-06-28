#ifndef SQL_DB_CONFIG
#define SQL_DB_CONFIG

#include <stdio.h>
#include "portable.h"

#ifdef  __cplusplus
extern "C" {
#endif


#define ERR_OK					0
#define ERR_LOCAL_MALLOC		1
#define ERR_WRONG_ALIGMENT		2
#define ERR_WRONG_INDEX			3
#define ERR_WRONG_SIZE			4
#define ERR_WRITE_FLASH			5
#define ERR_WRITE_EEPROM		6
#define ERR_WRITE_RAM			7
#define ERR_READ_FLASH			8
#define ERR_READ_EEPROM			9
#define ERR_READ_RAM			10
#define ERR_NO_FREE_SPACE		11
#define ERR_CRC					12
#define ERR_MAIN_READONLY		13
#define ERR_MAIN_SECTOR_FREE	14
#define ERR_START_SECTOR_FREE	15
#define ERR_ADDR_LEN			16
#define ERR_SIZE_LEN			17
#define ERR_SECTOR_FREE			18
#define ERR_SL_NULL				19
#define ERR_NO_MAIN				20


#define configUSE_SegmentCounter FALSE

/* Define the linked list structure.  This is used to link free blocks in order
of their memory address. */
typedef struct A_BLOCK_LINK
{
	UINT32_T pxNextFreeBlock;	/*<< The next free block in the list. */
	UINT32_T xBlockSize;		/*<< The size of the free block. */
}BlockLink_t;

typedef struct BLOCK_LINK
{
	UINT32_T pxCurrentAddr;
	struct A_BLOCK_LINK body;
}BlockLink;


//#define MAX_SECTOR_COUNT 2
#define SECTOR_FREE		0x00
#define SECTOR_FLASH	0x01
#define SECTOR_EEPROM	0x02
#define SECTOR_RAM		0x03

#define SECTOR_READONLY	0x10 //сектор можно только читать, не работает если сектор main
#define SECTOR_CRC		0x20 //каждая запись имеет CRC16
#define SECTOR_START	0x40 //информация о БД
#define SECTOR_MAIN     0x80 //информация о секторах, если сектор еще и START то он используется в работе и не может быть расширен
	

#define BYTES_1 0x01
#define BYTES_2 0x02
#define BYTES_3 0x03
#define BYTES_4 0x04

//1=		  127
//2=	   32 767
//3=	8 388 607
//4=2 147 483 647

//#define BLOCK_LINK_SIZE sizeof(BLOСK_OFFSET)+sizeof(BLOСK_OFFSET)+sizeof(BLOСK_SIZE)
//#define SECTOR_SIZE		(sizeof(UINT16_T)+sizeof(BLOСK_OFFSET)+2*(BLOCK_LINK_SIZE)+2*sizeof(BLOСK_SIZE)+sizeof(UINT16_T))
//#define DB_STRUCT_SIZE	sizeof(UINT8_T)+sizeof(UINT8_T)+(SECTOR_SIZE)*MAX_SECTOR_COUNT

//#define DB_HEADER_SIZE  (sizeof(UINT16_T)+sizeof(UINT8_T))
//#define BLOCK_LINK_SIZE (2*sizeof(UINT32_T))
//
//
//#if (configUSE_SegmentCounter==TRUE)
//	#define SECTOR_SIZE		(5*sizeof(UINT8_T)+5*sizeof(UINT32_T))
//#else
//	#define SECTOR_SIZE		(5*sizeof(UINT8_T)+4*sizeof(UINT32_T))
//#endif

//#if( configUSE_MALLOC_FAILED_HOOK == 1 )
//#endif



typedef struct A_SECTOR_INFO
{
		UINT8_T  StartAddrLen;			//длина поля адреса
		UINT8_T	 SectorSizeLen;			//длина поля размер
		UINT8_T  Type;					//тип сектора
		UINT8_T  bl_size;				//длина структуры блок линк с учетом выравнивания
		UINT8_T  ByteAligment;			//выравнивание ,всегда больше нуля
		UINT32_T StartAddr;				//смещение на сектор
		UINT32_T FreeBytesRemaining;	//остаток

		#if (configUSE_SegmentCounter==TRUE)
		UINT32_T xSegmentCounter;		//колличество сегментов
		#endif

		UINT32_T xStart_Addr;			//указатель на голову
		UINT32_T pxEnd_Addr;			//указатель на хвост
}SectorInfo;

typedef struct SECTOR_LIST
{
	UINT16_T	crc16; //генерить перед сохранением
	UINT8_T		sector_counter;
	
	struct A_SECTOR_INFO *sector;
}SectorList;

//добавить CRC
// SectorSize=FreeBytesRemaining=xSegmentCounter
// xStart_Addr=pxEnd_Addr=StartAddr


typedef struct SECTOR_CONFIG
{
	UINT8_T	index;
	UINT8_T type;
	UINT8_T StartAddrLen;
	UINT8_T SectorSizeLen;
	UINT32_T StartAddr;
	UINT32_T SectorSize;
	UINT8_T ByteAligment;
}SectorConfig;

//SIZE_T xHeapStructSize;    // создать переменную после открытия БД UINT16_T
//SIZE_T minimum_block_size; // просто умножить на 2 xHeapStructSize

#ifdef  __cplusplus
}
#endif


#endif