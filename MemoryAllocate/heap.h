#ifndef _HEAP
#define _HEAP

#include <stdio.h>
#include "portable.h"
#include "heap_hal.h"
#include "crc.h"

#ifdef  __cplusplus
extern "C" {
#endif

UINT8_T		sector_Init(UINT8_T index);		//+
UINT8_T		sector_Malloc(UINT8_T index, UINT32_T *addr, UINT32_T xWantedSize ); //+
UINT8_T		sector_Free(UINT8_T index, UINT32_T pv );	//+

UINT8_T		sector_Create(UINT8_T count);				//+
UINT8_T		sector_Insert(SectorConfig* config);		//+

UINT32_T	sector_GetFreeSize(UINT8_T index);			//+
UINT32_T	sector_GetSegmentCounter(UINT8_T index);	//+

//�������� ������� ������������ ������� ��������

//������� ������
UINT8_T sector_Delete(UINT8_T index); //�������� ���� type
//�������
UINT8_T sector_Open();
//������� ��
UINT8_T sector_Close(); //��������� ��������� db

#ifdef  __cplusplus
}
#endif

#endif