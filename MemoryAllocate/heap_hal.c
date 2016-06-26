#include <stdlib.h>
#include "portable.h"
#include "sql_db_config.h"

UINT8_T ucHeap[2048];

/*-----------------------------------------------------------*/
UINT8_T sector_write(UINT8_T sector, UINT32_T addr, void *data, UINT16_T size)
{
	UINT16_T i;

	if(sector==0)
	{
		for(i=0;i<size;i++ )
		{
			*(ucHeap+addr+i)=*((UINT8_T*)data+i);
		}
	}
	else
	{
	}

	return ERR_OK;
}

/*-----------------------------------------------------------*/
UINT8_T sector_read(UINT8_T sector, UINT32_T addr, void *data, UINT16_T size)
{
	UINT16_T i;

	if(sector==0)
	{
		for(i=0;i<size;i++ )
		{
			*((UINT8_T*)data+i)=*(ucHeap+addr+i);
		}
	}
	else
	{
	}

	return ERR_OK;
}


/*-----------------------------------------------------------*/
void *local_malloc(size_t size)
{
	return malloc(size);
}

/*-----------------------------------------------------------*/
void local_free(void *block)
{
	free(block);
}