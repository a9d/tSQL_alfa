// MemoryAllocate.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include "heap.h"
#include "sql_db_config.h"
#include <memory.h>


extern UINT8_T ucHeap[2048];
extern UINT8_T ucHeap1[2048];
//параметры всех служебных структур заранее определены

//SIZE_T xHeapStructSize;
//SIZE_T minimum_block_size; // просто умножить на 2 xHeapStructSize
//BlockLink_t xStart;  
//BlockLink_t pxEnd;
//SIZE_T xFreeBytesRemaining;
//SIZE_T xMinimumEverFreeBytesRemaining; //не сохранять
//SIZE_T xSegmentCounter;
//SIZE_T xBlockAllocatedBit ;
//UINT16_T byte_aligment;


void ApplicationSectorPrepareHook(void)
{
	//подготовка памяти к размещению секторов
}

void ApplicationSectorOpenHook(UINT8_T index, UINT32_T start_addr,UINT32_T size)
{
	//подготовка сектора
}

void ApplicationSectorDelete(UINT8_T index, UINT32_T start_addr, UINT32_T size)
{
	//удаление заданного сектора
}


int _tmain(int argc, _TCHAR* argv[])
{
	int i;
	int size;
	SectorConfig config;
	UINT32_T test;
	UINT32_T addr1,addr2,addr3,addr4,addr5;
	UINT8_T buf[]={1,2,3,4,5,6};
	UINT32_T addr11;


	sector_Create(2);

	config.index=0;
	config.type=(SECTOR_MAIN|SECTOR_FLASH);
	config.ByteAligment=2;
	config.StartAddr=0;
	config.StartAddrLen=BYTES_2;
	config.SectorSize=2000;
	config.SectorSizeLen=BYTES_2;

	sector_ConfigCheck(&config);	
	sector_Insert(&config);
	sector_GetSectorConfig(0,&config);

	//
	config.index=1;
	config.type=(SECTOR_EEPROM);
	config.ByteAligment=1;
	config.StartAddr=0;
	config.StartAddrLen=BYTES_4;
	config.SectorSize=2000;
	config.SectorSizeLen=BYTES_4;

	sector_ConfigCheck(&config);
	sector_Insert(&config); //портится куча
	sector_GetSectorConfig(1,&config);

	//sector_Malloc(0,&addr1,6);
	//sector_write(0,addr1,(void*)buf,6);

	sector_Malloc(1,&addr11,6);
	sector_write(1,addr11,(void*)buf,6);

	//sector_Malloc(0,&addr2,6);
	//sector_write(0,addr2,(void*)buf,6);

	//sector_Malloc(0,&addr3,6);
	//sector_write(0,addr3,(void*)buf,6);

	//sector_Malloc(0,&addr4,6);
	//sector_write(0,addr4,(void*)buf,6);

	//sector_Malloc(0,&addr5,6);
	//sector_write(0,addr5,(void*)buf,6);

	//sector_Free(0,addr2);
	//sector_Free(0,addr4);
	//sector_Free(0,addr5);
	//sector_Free(0,addr3); 

	//sector_Malloc(0,&addr2,14);

	sector_GetFreeSize(0);
	//sector_GetSegmentCounter(0);

	sector_GetFreeSize(1);

	config.index=2;
	config.type=(SECTOR_FLASH);
	config.ByteAligment=2;
	config.StartAddr=0;
	config.StartAddrLen=BYTES_2;
	config.SectorSize=2000;
	config.SectorSizeLen=BYTES_2;
	sector_ConfigCheck(&config);

	//sector_Delete(1);
	//sector_AddNewSector(&config);

	sector_Close();
	sector_Open(0, 2 ,0);
	//sector_GetSegmentCounter(1);

	return 0;
}

