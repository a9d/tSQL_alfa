#include <stdlib.h>
#include "portable.h"

UINT8_T ucHeap[2048];

/*-----------------------------------------------------------*/
//не передавать aligment
void sector_write(UINT8_T sector, UINT32_T addr, void *data, UINT16_T size,UINT16_T aligment)
{
	UINT16_T i,aligment_size;

	if(sector==0)
	{
		for(i=0;i<size;i++ )
		{
			*(ucHeap+addr+i)=*((UINT8_T*)data+i);
		}

		aligment_size=size+(aligment-1)& ~(aligment-1);

		for(i;i<aligment_size;i++)
		{
			*(ucHeap+addr+i)=0;
		}	
	}
	else
	{
	}
}

/*-----------------------------------------------------------*/
void sector_read(UINT8_T sector, UINT32_T addr, void *data, UINT16_T size)
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