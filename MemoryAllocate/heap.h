#ifndef _HEAP
#define _HEAP

#include <stdio.h>
#include "portable.h"
#include "heap_hal.h"
#include "crc.h"
#include "sql_db_config.h"

#ifdef  __cplusplus
extern "C" {
#endif

UINT8_T		sector_Init(UINT8_T index);		//+
UINT8_T		sector_Malloc(UINT8_T index, UINT32_T *addr, UINT32_T xWantedSize ); //+
UINT8_T		sector_Free(UINT8_T index, UINT32_T pv );	//+

UINT8_T		sector_Create(UINT8_T count);				//+
UINT8_T		sector_Insert(SectorConfig* config);		//+

UINT32_T	sector_GetFreeSize(UINT8_T index);			//+

#if (configUSE_SegmentCounter==TRUE)
UINT32_T	sector_GetSegmentCounter(UINT8_T index);	//+
#endif

UINT8_T		sector_ConfigCheck(SectorConfig* config);  //+

void		sector_ResourceFree();		//+

UINT8_T		sector_GetSectorConfig(UINT8_T index, SectorConfig* config); //вернуть конфигурацию сектора //+

UINT8_T sector_Close(); //сохранить структуру db. Освободить ресурсы

UINT8_T sector_MainSave(); //сохранить сектор main

//удалить сектор
UINT8_T sector_Delete(UINT8_T index); //очистить поле type //+

//открыть


UINT8_T sector_AddNewSector(SectorConfig* config);//добавление нового сектора, сохранение таблицы и открытие заново

UINT8_T sector_Open(UINT8_T index, UINT8_T Aligment ,UINT32_T addr); //открыть сектор main . Подготовить все сектора к работе

#ifdef  __cplusplus
}
#endif

#endif