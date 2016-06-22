// MemoryAllocate.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include "heap.h"
#include "sql_db_config.h"
#include <memory.h>


extern UINT8_T ucHeap[2048];
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

void ApplicationSectorStartHook(UINT8_T index, UINT32_T start_addr,UINT32_T size)
{
	//подготовка сектора
}

int _tmain(int argc, _TCHAR* argv[])
{
	int i;
	int size;
	SectorConfig config;

	sector_Create(2);

	config.index=0;
	config.type=(SECTOR_MAIN|SECTOR_START|SECTOR_FLASH);
	config.ByteAligment=2;
	
	config.StartAddr=0;
	config.StartAddrLen=BYTES_2;

	config.SectorSize=2048;
	config.SectorSizeLen=BYTES_2;

	sector_Insert(&config);



	return 0;

	//int i;
	//UINT32_T addr1,addr2,addr3,addr4,addr5;
	//UINT16_T test;

	//DataBase g;

	////g.sector[0].i;

	//prvHeapInit((UINT32_T)0,200,1,0);

	//addr1 = pvPortMalloc( 5 );
	//
	//for(i=0;i<5;i++)
	//{
	//	test=i+1;
	//	heap_write(0, addr1+i, &test, 1,1);
	//}

	//addr2 = pvPortMalloc( 5 );

	//for(i=0;i<5;i++)
	//{
	//	test=i+1;
	//	heap_write(0, addr2+i, &test, 1,1);
	//}

	//addr3 = pvPortMalloc( 5 );

	//for(i=0;i<5;i++)
	//{
	//	test=i+1;
	//	heap_write(0, addr3+i, &test, 1,1);
	//}

	//addr4 = pvPortMalloc( 5 );

	//for(i=0;i<5;i++)
	//{
	//	test=i+1;
	//	heap_write(0, addr4+i, &test, 1,1);
	//}

	//addr5 = pvPortMalloc( 5 );

	//for(i=0;i<5;i++)
	//{
	//	test=i+1;
	//	heap_write(0, addr5+i, &test, 1,1);
	//}


	//vPortFree(addr2);
	//vPortFree(addr4);
	//vPortFree(addr5);
	//vPortFree(addr3);

	//addr2 =pvPortMalloc( 15 );

	//for(i=0;i<15;i++)
	//{
	//	test=i+1;
	//	heap_write(0, addr2+i, &test, 1,1);
	//}

	//xPortGetFreeHeapSize();
	////xPortGetMinimumEverFreeHeapSize();

	//return 0;
}

