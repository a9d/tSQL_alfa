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
#define ERR_WRONG_SIZE			11
#define ERR_WRITE_FLASH			4
#define ERR_WRITE_EEPROM		5
#define ERR_WRITE_RAM			6
#define ERR_READ_FLASH			7
#define ERR_READ_EEPROM			8
#define ERR_READ_RAM			9
#define ERR_NO_FREE_SPACE		10
#define ERR_CRC					12



/* Define the linked list structure.  This is used to link free blocks in order
of their memory address. */
typedef struct A_BLOCK_LINK
{
	UINT32_T pxNextFreeBlock;	/*<< The next free block in the list. */
	UINT32_T xBlockSize;		/*<< The size of the free block. */
};

typedef struct A_BLOCK_LINK1
{
	//UINT16_T crc16;
	UINT32_T pxNextFreeBlock;	/*<< The next free block in the list. */
	UINT32_T xBlockSize;		/*<< The size of the free block. */
}BlockLink_tt;

typedef struct BLOCK_LINK
{
	UINT32_T pxCurrentAddr;
	struct A_BLOCK_LINK body;
}BlockLink_t;


//#define MAX_SECTOR_COUNT 2

#define SECTOR_FLASH	0x00
#define SECTOR_EEPROM	0x01
#define SECTOR_RAM		0x02

#define SECTOR_READONLY	0x10 //������ ����� ������ ������, �� �������� ���� ������ main
#define SECTOR_CRC		0x20 //������ ������ ����� CRC8 ��� CRC16
#define SECTOR_START	0x40 //���������� � ��
#define SECTOR_MAIN     0x80 //���������� � ��������, ���� ������ ��� � START �� �� ������������ � ������ � �� ����� ���� ��������
	

#define BYTES_1 0x01
#define BYTES_2 0x02
#define BYTES_3 0x03
#define BYTES_4 0x04

//1=		  127
//2=	   32 767
//3=	8�388�607
//4=2�147�483�647

//#define BLOCK_LINK_SIZE sizeof(BLO�K_OFFSET)+sizeof(BLO�K_OFFSET)+sizeof(BLO�K_SIZE)
//#define SECTOR_SIZE		(sizeof(UINT16_T)+sizeof(BLO�K_OFFSET)+2*(BLOCK_LINK_SIZE)+2*sizeof(BLO�K_SIZE)+sizeof(UINT16_T))
//#define DB_STRUCT_SIZE	sizeof(UINT8_T)+sizeof(UINT8_T)+(SECTOR_SIZE)*MAX_SECTOR_COUNT

#define DB_HEADER_SIZE  (sizeof(UINT16_T)+sizeof(UINT8_T))
#define BLOCK_LINK_SIZE (2*sizeof(UINT32_T))
#define SECTOR_SIZE		(4*sizeof(UINT8_T)+sizeof(UINT16_T)+6*sizeof(UINT32_T))


typedef struct DATA_BASE
{
	UINT16_T	crc16; //�������� ����� �����������
	UINT8_T		sector_counter;
	
	struct {
		//UINT16_T FieldSize;				//������ (� ������): (������� �������)���������,(������� �������)������ ������, (����)��� �������
		UINT8_T  StartAddrLen;			//����� ���� ������
		UINT8_T	 SectorSizeLen;			//����� ���� ������
		UINT8_T  Type;					//��� �������
		UINT8_T  bl_size;				//����� ��������� ���� ���� � ������ ������������
		UINT16_T ByteAligment;			//������������ ,������ ������ ����
		UINT32_T StartAddr;				//�������� �� ������
		UINT32_T SectorSize;			//������ �������
		UINT32_T FreeBytesRemaining;	//�������
		UINT32_T xSegmentCounter;		//����������� ���������
		UINT32_T xStart_Addr;			//��������� �� ������
		UINT32_T pxEnd_Addr;			//��������� �� �����
	}sector[];
}DataBase;

//�������� CRC
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
	UINT16_T ByteAligment;
}SectorConfig;

//SIZE_T xHeapStructSize;    // ������� ���������� ����� �������� �� UINT16_T
//SIZE_T minimum_block_size; // ������ �������� �� 2 xHeapStructSize

#ifdef  __cplusplus
}
#endif


#endif