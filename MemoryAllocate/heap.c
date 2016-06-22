#include <stdlib.h>
#include <memory.h>
#include "heap.h"
#include "sql_db_config.h"

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE		( ( UINT8_T ) 8 )

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert );

extern void ApplicationSectorPrepareHook( void ); //подготовка всех секторов
extern void ApplicationSectorStartHook(UINT8_T index, UINT32_T start_addr,UINT32_T size); //подготовка сектора

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
block must by correctly byte aligned. */
static SIZE_T xHeapStructSize;

/* Block sizes must not get too small. */
static SIZE_T minimum_block_size;

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart, pxEnd;

/* Keeps track of the number of free bytes remaining, but says nothing about
fragmentation. */
static UINT32_T xFreeBytesRemaining = 0U;
static UINT32_T xSegmentCounter = 0U;

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
member of an BlockLink_t structure is set then the block belongs to the
application.  When the bit is free the block is still part of the free heap
space. */
static SIZE_T xBlockAllocatedBit = 0U;

UINT16_T byte_aligment;

static	UINT8_T malloc_init_state=FALSE;

DataBase *db;

/*-----------------------------------------------------------*/
UINT32_T sector_Malloc( SIZE_T xWantedSize )
{
BlockLink_t pxBlock, *pxPreviousBlock, pxNewBlockLink; 
UINT32_T pvReturn = 0;

	/* If this is the first call to malloc then the heap will require
	initialisation to setup the list of free blocks. */
	if(malloc_init_state==FALSE)
	{
		return 0;
	}

	/* The wanted size is increased so it can contain a BlockLink_t
	structure in addition to the requested amount of bytes. */
	if( xWantedSize > 0 )
	{
		xWantedSize += xHeapStructSize;

		/* Ensure that blocks are always aligned to the required number
		of bytes. */
		if( ( xWantedSize & (byte_aligment-1) ) != 0x00 )
		{
			/* Byte alignment required. */
			xWantedSize += ( byte_aligment - ( xWantedSize & (byte_aligment-1) ) ) ;
		}
	}

	if( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining ) )
	{
		/* Traverse the list from the start	(lowest address) block until
		one	of adequate size is found. */
		pxPreviousBlock = &xStart;
			
		sector_read(0,xStart.body.pxNextFreeBlock,(void*)&pxBlock.body,xHeapStructSize);
		pxBlock.pxCurrentAddr=xStart.body.pxNextFreeBlock;

		while((pxBlock.body.xBlockSize<xWantedSize)&&(pxBlock.body.pxNextFreeBlock!=0))
		{
			pxPreviousBlock = &pxBlock;

			pxBlock.pxCurrentAddr=pxBlock.body.pxNextFreeBlock;
			sector_read(0,pxBlock.pxCurrentAddr,(void*)&pxBlock.body,xHeapStructSize);;
		}

		/* If the end marker was reached then a block of adequate size
		was	not found. */
		if(pxBlock.pxCurrentAddr!=pxEnd.pxCurrentAddr)
		{
			/* Return the memory space pointed to - jumping over the
			BlockLink_t structure at its start. */
			pvReturn = pxPreviousBlock->body.pxNextFreeBlock+xHeapStructSize;

			/* This block is being returned for use so must be taken out
			of the list of free blocks. */
			pxPreviousBlock->body.pxNextFreeBlock=pxBlock.body.pxNextFreeBlock;
			sector_write(0,pxPreviousBlock->pxCurrentAddr,(void*)&pxPreviousBlock->body,xHeapStructSize,byte_aligment);

			/* If the block is larger than required it can be split into
			two. */
			if( ( pxBlock.body.xBlockSize - xWantedSize ) > minimum_block_size)		
			{
				/* This block is to be split into two.  Create a new
				block following the number of bytes requested. The void
				cast is used to prevent byte alignment warnings from the
				compiler. */
				pxNewBlockLink.pxCurrentAddr=pxBlock.pxCurrentAddr+xWantedSize;

				/* Calculate the sizes of two blocks split from the
				single block. */
				pxNewBlockLink.body.xBlockSize=pxBlock.body.xBlockSize-xWantedSize;
				pxNewBlockLink.body.pxNextFreeBlock=0;

				pxBlock.body.xBlockSize=xWantedSize;
				//writeMalloc(0, 0, pxBlock.pxCurrentAddr, (void*)&pxBlock.body,xHeapStructSize);

				/* Insert the new block into the list of free blocks. */
				prvInsertBlockIntoFreeList(&pxNewBlockLink);
				xSegmentCounter++;
			}

			xFreeBytesRemaining -= pxBlock.body.xBlockSize;

			/* The block is being returned - it is allocated and owned
			by the application and has no "next" block. */
			pxBlock.body.xBlockSize |= xBlockAllocatedBit;									//!!!!!!!!!!!!!!!!!!!!
			pxBlock.body.pxNextFreeBlock = 0;
			sector_write(0, pxBlock.pxCurrentAddr, (void*)&pxBlock.body,xHeapStructSize,byte_aligment);
		}
	}

	#if( configUSE_MALLOC_FAILED_HOOK == 1 )
	{
		if( pvReturn == NULL )
		{
			extern void vApplicationMallocFailedHook( void );
			vApplicationMallocFailedHook();
		}
	}
	#endif

	return pvReturn;
}

/*-----------------------------------------------------------*/
void sector_Free( UINT32_T pv )
{
UINT32_T puc=pv;
BlockLink_t pxLink;

	if( pv != 0 )
	{
		/* The memory being freed will have an BlockLink_t structure immediately
		before it. */
		puc -= xHeapStructSize;

		/* This casting is to keep the compiler from issuing warnings. */
		pxLink.pxCurrentAddr=puc;
		sector_read(0, puc, (void*)&pxLink.body, xHeapStructSize);

		if( ( pxLink.body.xBlockSize & xBlockAllocatedBit ) != 0 )								//!!!!!!!!!!!!!!
		{
			if( pxLink.body.pxNextFreeBlock == 0 )
			{
				/* The block is being returned to the heap - it is no longer
				allocated. */
				pxLink.body.xBlockSize&= ~xBlockAllocatedBit;									//!!!!!!!!!!!!!!!!!
				
				/* Add this block to the list of free blocks. */
				xFreeBytesRemaining +=pxLink.body.xBlockSize;
				prvInsertBlockIntoFreeList(&pxLink);
			}
		}
	}
}

/*-----------------------------------------------------------*/
UINT32_T sector_GetFreeSize( void )
{
	return xFreeBytesRemaining;
}

/*-----------------------------------------------------------*/
UINT32_T sector_GetSegmentCounter( void )
{
	return xSegmentCounter;
}

/*-----------------------------------------------------------*/
void sector_Init(UINT32_T addr, UINT32_T size, UINT16_T aligment, UINT16_T aligment_mask)
{
BlockLink_t pxFirstFreeBlock;
BLOСK_OFFSET pucAlignedHeap;
BLOСK_OFFSET ulAddress;
BLOСK_SIZE	xTotalHeapSize;

	byte_aligment=aligment;

	xHeapStructSize	= ( sizeof( BLOСK_OFFSET ) + sizeof( BLOСK_SIZE ) + ( byte_aligment  -  1 ) ) & ~(byte_aligment-1);
	minimum_block_size =  xHeapStructSize<<1;

	/* Ensure the heap starts on a correctly aligned boundary. */
	ulAddress = addr+xHeapStructSize;
	xTotalHeapSize = size-xHeapStructSize;

	if( ( ulAddress & (byte_aligment-1) ) != 0 )
	{
		ulAddress += byte_aligment-1 ;
		ulAddress &=  ~( ( UINT32_T ) (byte_aligment-1) );
		xTotalHeapSize -= ulAddress - addr;
	}

	pucAlignedHeap = ulAddress;

	/* xStart is used to hold a pointer to the first item in the list of free
	blocks.  The void cast is used to prevent compiler warnings. */
	xStart.pxCurrentAddr=ulAddress-xHeapStructSize;
	xStart.body.pxNextFreeBlock = pucAlignedHeap;
	xStart.body.xBlockSize = 0;
	sector_write(0,xStart.pxCurrentAddr,(void*)&xStart.body,xHeapStructSize,byte_aligment);

	/* pxEnd is used to mark the end of the list of free blocks and is inserted
	at the end of the heap space. */
	ulAddress = ( ( UINT32_T ) pucAlignedHeap ) + xTotalHeapSize;
	ulAddress -= xHeapStructSize;
	ulAddress &= ~( ( UINT32_T ) (byte_aligment-1));
	
	pxEnd.body.pxNextFreeBlock=0;
	pxEnd.body.xBlockSize=0;
	pxEnd.pxCurrentAddr=ulAddress;
	sector_write(0,pxEnd.pxCurrentAddr,(void*)&pxEnd.body, xHeapStructSize,byte_aligment);
	
	/* To start with there is a single free block that is sized to take up the
	entire heap space, minus the space taken by pxEnd. */
	pxFirstFreeBlock.pxCurrentAddr=pucAlignedHeap;
	pxFirstFreeBlock.body.xBlockSize= ulAddress - pxFirstFreeBlock.pxCurrentAddr;
	pxFirstFreeBlock.body.pxNextFreeBlock= pxEnd.pxCurrentAddr;
	sector_write(0,pxFirstFreeBlock.pxCurrentAddr,(void*)&pxFirstFreeBlock.body, xHeapStructSize,byte_aligment);

	/* Only one block exists - and it covers the entire usable heap space. */
	xFreeBytesRemaining = pxFirstFreeBlock.body.xBlockSize;

	xSegmentCounter=2;

	/* Work out the position of the top bit in a size_t variable. */
	xBlockAllocatedBit = ( ( BLOСK_SIZE ) 1 ) << ( ( sizeof( BLOСK_SIZE ) * heapBITS_PER_BYTE ) - 1 );

	malloc_init_state=TRUE;
}

/*-----------------------------------------------------------*/
static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert )
{
BlockLink_t *pxIterator,pxIteratorBuf,pxBlockToInsertBuf;

	/* Iterate through the list until a block is found that has a higher address
	than the block being inserted. */
	pxIterator = &xStart;

	while(pxIterator->body.pxNextFreeBlock < pxBlockToInsert->pxCurrentAddr)
	{
		pxIteratorBuf.pxCurrentAddr=pxIterator->body.pxNextFreeBlock;
		sector_read(0, pxIteratorBuf.pxCurrentAddr, (void*)&pxIteratorBuf.body, xHeapStructSize);
		pxIterator = &pxIteratorBuf;
	}

	/* Do the block being inserted, and the block it is being inserted after
	make a contiguous block of memory? */
	if((pxIterator->pxCurrentAddr+pxIterator->body.xBlockSize)==pxBlockToInsert->pxCurrentAddr)
	{
		pxIterator->body.xBlockSize+=pxBlockToInsert->body.xBlockSize;
		pxBlockToInsert = pxIterator;

		xSegmentCounter--;
	}

	/* Do the block being inserted, and the block it is being inserted before
	make a contiguous block of memory? */
	if((pxBlockToInsert->pxCurrentAddr+pxBlockToInsert->body.xBlockSize)==pxIterator->body.pxNextFreeBlock)
	{
		if(pxIterator->body.pxNextFreeBlock!=pxEnd.pxCurrentAddr)
		{
			/* Form one big block from the two blocks. */
			pxBlockToInsertBuf.pxCurrentAddr=pxIterator->body.pxNextFreeBlock;
			sector_read(0, pxBlockToInsertBuf.pxCurrentAddr, (void*)&pxBlockToInsertBuf.body, xHeapStructSize);

			pxBlockToInsert->body.xBlockSize=pxBlockToInsertBuf.body.xBlockSize;
			pxBlockToInsert->body.pxNextFreeBlock=pxBlockToInsertBuf.body.pxNextFreeBlock;

			xSegmentCounter--;
		}
		else
		{
			pxBlockToInsert->body.pxNextFreeBlock=pxEnd.pxCurrentAddr;
		}
	}
	else
	{
		pxBlockToInsert->body.pxNextFreeBlock=pxIterator->body.pxNextFreeBlock;
	}

	/* If the block being inserted plugged a gab, so was merged with the block
	before and the block after, then it's pxNextFreeBlock pointer will have
	already been set, and should not be set here as that would make it point
	to itself. */
	if( pxIterator->pxCurrentAddr != pxBlockToInsert->pxCurrentAddr )
	{
		pxIterator->body.pxNextFreeBlock = pxBlockToInsert->pxCurrentAddr;
		sector_write(0, pxIterator->pxCurrentAddr, (void*)&pxIterator->body, xHeapStructSize,byte_aligment);
	}
	sector_write(0,pxBlockToInsert->pxCurrentAddr,(void*)&pxBlockToInsert->body,xHeapStructSize,byte_aligment);
}

/*-----------------------------------------------------------*/
UINT8_T sector_Create(UINT8_T count)
{
	UINT16_T size;

	size=DB_HEADER_SIZE+SECTOR_SIZE*count;

	db=(DataBase*)local_malloc(size);

	if(db!=NULL)
	{
		memset((void*)db,0x00,size);

		db->sector_counter=count;

		ApplicationSectorPrepareHook();
		return 0;
	}
	return 1;
}

UINT8_T sector_Insert(SectorConfig* config)
{
	UINT16_T size;

	if(db!=NULL)
	{
		if(config->ByteAligment==0)
			return 1;

		if(config->index>(db->sector_counter-1))
			return 1;

		db->sector[config->index].FieldSize=(((UINT16_T)config->StartAddrLen)<<12) | (((UINT16_T)config->SectorSizeLen)<<8) | (UINT16_T)config->type;
		db->sector[config->index].StartAddr=config->StartAddr;
		db->sector[config->index].SectorSize=config->SectorSize;
		db->sector[config->index].FreeBytesRemaining=config->SectorSize;
		db->sector[config->index].ByteAligment=config->ByteAligment;

		//если сектор обычный ничего не коректировать
		if((config->type & SECTOR_MAIN)>0)
		{
			//с учетом выравнивания данного сектора!!!

			size=((DB_HEADER_SIZE+SECTOR_SIZE*db->sector_counter)+( config->ByteAligment-1)) & ~(config->ByteAligment-1);

			//если сектор только SECTOR_MAIN, скоректировать FreeBytesRemaining
			db->sector[config->index].FreeBytesRemaining=config->SectorSize-size;

			if((config->type & SECTOR_START)>0)
			{
				//если сектор SECTOR_MAIN и SECTOR_START, скоректировать FreeBytesRemaining и xStart_Addr
				db->sector[config->index].xStart_Addr=size;
			}
		}

		//вызвать хук
		ApplicationSectorStartHook(config->index, config->StartAddr,config->SectorSize);

		//инициализировать данный сектор
		
		return 0;
	}
	return 1;
}


//удалить сектор
void sector_Delete()
{
}

//открыть
void sector_Open()
{
}

//закрыть бд
void sector_Close()
{
}

