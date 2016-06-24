#ifndef HEAP_HAL
#define HEAP_HAL

#include <stdio.h>
#include "portable.h"
#include "sql_db_config.h"

#ifdef  __cplusplus
extern "C" {
#endif

	//ПРОБЛЕМА УЧЕТА CRC!!!! CRC генерируется на более высоком уровне
void sector_write(UINT8_T sector, UINT32_T addr, void *data, UINT16_T size,UINT16_T aligment);
void sector_read(UINT8_T sector, UINT32_T addr, void *data, UINT16_T size);

UINT8_T _sector_write(UINT8_T sector, UINT32_T addr, void *data, UINT16_T size);
UINT8_T _sector_read(UINT8_T sector, UINT32_T addr, void *data, UINT16_T size);

void *local_malloc(size_t size);
void local_free(void *block);

#ifdef  __cplusplus
}
#endif

#endif