#ifndef _HEAP
#define _HEAP

#include <stdio.h>
#include "portable.h"
#include "heap_hal.h"
#include "crc.h"
//#include "sql_db_config.h"

#ifdef  __cplusplus
extern "C" {
#endif

void		sector_Init(UINT32_T addr, UINT32_T size, UINT16_T aligment, UINT16_T aligment_mask);
UINT32_T	sector_Malloc( SIZE_T xWantedSize );
void		sector_Free( UINT32_T pv );

UINT8_T		sector_Init1(UINT8_T index);
UINT8_T		sector_Malloc1(UINT8_T index,UINT32_T *addr, UINT32_T xWantedSize );
//void		sector_Free( UINT32_T pv );

UINT8_T		sector_Create(UINT8_T count);
UINT8_T		sector_Insert(SectorConfig* config);

UINT32_T	sector_GetFreeSize( void );// GetFreeSectorSize( void );
UINT32_T	sector_GetSegmentCounter( void );

//добавить функцию освобождения занятых ресурсов

#ifdef  __cplusplus
}
#endif

#endif