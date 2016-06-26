#include <stdlib.h>
#include <memory.h>
#include "heap.h"
#include "sql_db_config.h"

/* Assumes 8bit bytes! */
//#define heapBITS_PER_BYTE		( ( UINT8_T ) 8 )

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
//static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert );

extern void ApplicationSectorPrepareHook( void ); //подготовка всех секторов
extern void ApplicationSectorStartHook(UINT8_T index, UINT32_T start_addr,UINT32_T size); //подготовка сектора

/////vApplicationMallocFailedHook
///*-----------------------------------------------------------*/
//
///* The size of the structure placed at the beginning of each allocated memory
//block must by correctly byte aligned. */
//static SIZE_T xHeapStructSize;
//
///* Block sizes must not get too small. */
//static SIZE_T minimum_block_size;
//
///* Create a couple of list links to mark the start and end of the list. */
////static BlockLink_t xStart, pxEnd;
//
///* Keeps track of the number of free bytes remaining, but says nothing about
//fragmentation. */
//static UINT32_T xFreeBytesRemaining = 0U;
//static UINT32_T xSegmentCounter = 0U;
//
///* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
//member of an BlockLink_t structure is set then the block belongs to the
//application.  When the bit is free the block is still part of the free heap
//space. */
//static SIZE_T xBlockAllocatedBit = 0U;
//
//UINT16_T byte_aligment;
//
//static	UINT8_T malloc_init_state=FALSE;


//---------------------------- NEW -------------------------------------------
UINT8_T prvInsertBlockIntoFreeList(UINT8_T index, BlockLink *pxBlockToInsert );
UINT8_T WriteBlockLink(UINT8_T index, BlockLink *bl);
UINT8_T ReadBlockLink(UINT8_T index, BlockLink *bl);

DataBase *sl;						//описание секторов базы данных
BlockLink *xStart, *pxEnd;	//заголовки сегментов


/*-----------------------------------------------------------*/
//UINT32_T sector_Malloc( SIZE_T xWantedSize )
//{
//BlockLink_t pxBlock, *pxPreviousBlock, pxNewBlockLink; 
//UINT32_T pvReturn = 0;
//
//	/* If this is the first call to malloc then the heap will require
//	initialisation to setup the list of free blocks. */
//	if(malloc_init_state==FALSE)
//	{
//		return 0;
//	}
//
//	/* The wanted size is increased so it can contain a BlockLink_t
//	structure in addition to the requested amount of bytes. */
//	if( xWantedSize > 0 )
//	{
//		xWantedSize += xHeapStructSize;
//
//		/* Ensure that blocks are always aligned to the required number
//		of bytes. */
//		if( ( xWantedSize & (byte_aligment-1) ) != 0x00 )
//		{
//			/* Byte alignment required. */
//			xWantedSize += ( byte_aligment - ( xWantedSize & (byte_aligment-1) ) ) ;
//		}
//	}
//
//	if( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining ) )
//	{
//		/* Traverse the list from the start	(lowest address) block until
//		one	of adequate size is found. */
//		pxPreviousBlock = &xStart;
//			
//		sector_read(0,xStart.body.pxNextFreeBlock,(void*)&pxBlock.body,xHeapStructSize);
//		pxBlock.pxCurrentAddr=xStart.body.pxNextFreeBlock;
//
//		while((pxBlock.body.xBlockSize<xWantedSize)&&(pxBlock.body.pxNextFreeBlock!=0))
//		{
//			pxPreviousBlock = &pxBlock;
//
//			pxBlock.pxCurrentAddr=pxBlock.body.pxNextFreeBlock;
//			sector_read(0,pxBlock.pxCurrentAddr,(void*)&pxBlock.body,xHeapStructSize);;
//		}
//
//		/* If the end marker was reached then a block of adequate size
//		was	not found. */
//		if(pxBlock.pxCurrentAddr!=pxEnd.pxCurrentAddr)
//		{
//			/* Return the memory space pointed to - jumping over the
//			BlockLink_t structure at its start. */
//			pvReturn = pxPreviousBlock->body.pxNextFreeBlock+xHeapStructSize;
//
//			/* This block is being returned for use so must be taken out
//			of the list of free blocks. */
//			pxPreviousBlock->body.pxNextFreeBlock=pxBlock.body.pxNextFreeBlock;
//			sector_write(0,pxPreviousBlock->pxCurrentAddr,(void*)&pxPreviousBlock->body,xHeapStructSize,byte_aligment);
//
//			/* If the block is larger than required it can be split into
//			two. */
//			if( ( pxBlock.body.xBlockSize - xWantedSize ) > minimum_block_size)		
//			{
//				/* This block is to be split into two.  Create a new
//				block following the number of bytes requested. The void
//				cast is used to prevent byte alignment warnings from the
//				compiler. */
//				pxNewBlockLink.pxCurrentAddr=pxBlock.pxCurrentAddr+xWantedSize;
//
//				/* Calculate the sizes of two blocks split from the
//				single block. */
//				pxNewBlockLink.body.xBlockSize=pxBlock.body.xBlockSize-xWantedSize;
//				pxNewBlockLink.body.pxNextFreeBlock=0;
//
//				pxBlock.body.xBlockSize=xWantedSize;
//				//writeMalloc(0, 0, pxBlock.pxCurrentAddr, (void*)&pxBlock.body,xHeapStructSize);
//
//				/* Insert the new block into the list of free blocks. */
//				prvInsertBlockIntoFreeList(&pxNewBlockLink);
//				xSegmentCounter++;
//			}
//
//			xFreeBytesRemaining -= pxBlock.body.xBlockSize;
//
//			/* The block is being returned - it is allocated and owned
//			by the application and has no "next" block. */
//			pxBlock.body.xBlockSize |= xBlockAllocatedBit;									//!!!!!!!!!!!!!!!!!!!!
//			pxBlock.body.pxNextFreeBlock = 0;
//			sector_write(0, pxBlock.pxCurrentAddr, (void*)&pxBlock.body,xHeapStructSize,byte_aligment);
//		}
//	}
//
//	#if( configUSE_MALLOC_FAILED_HOOK == 1 )
//	{
//		if( pvReturn == NULL )
//		{
//			extern void vApplicationMallocFailedHook( void );
//			vApplicationMallocFailedHook();
//		}
//	}
//	#endif
//
//	return pvReturn;
//}

/*-----------------------------------------------------------*/
//void sector_Free( UINT32_T pv )
//{
//UINT32_T puc=pv;
//BlockLink_t pxLink;
//
//	if( pv != 0 )
//	{
//		/* The memory being freed will have an BlockLink_t structure immediately
//		before it. */
//		puc -= xHeapStructSize;
//
//		/* This casting is to keep the compiler from issuing warnings. */
//		pxLink.pxCurrentAddr=puc;
//		sector_read(0, puc, (void*)&pxLink.body, xHeapStructSize);
//
//		if( ( pxLink.body.xBlockSize & xBlockAllocatedBit ) != 0 )								//!!!!!!!!!!!!!!
//		{
//			if( pxLink.body.pxNextFreeBlock == 0 )
//			{
//				/* The block is being returned to the heap - it is no longer
//				allocated. */
//				pxLink.body.xBlockSize&= ~xBlockAllocatedBit;									//!!!!!!!!!!!!!!!!!
//				
//				/* Add this block to the list of free blocks. */
//				xFreeBytesRemaining +=pxLink.body.xBlockSize;
//				prvInsertBlockIntoFreeList(&pxLink);
//			}
//		}
//	}
//}

/*-----------------------------------------------------------*/
UINT32_T sector_GetFreeSize(UINT8_T index)
{
	return sl->sector[index].FreeBytesRemaining;
}

/*-----------------------------------------------------------*/
UINT32_T sector_GetSegmentCounter(UINT8_T index)
{
	return sl->sector[index].xSegmentCounter;
}

/*-----------------------------------------------------------*/
//void sector_Init(UINT32_T addr, UINT32_T size, UINT16_T aligment, UINT16_T aligment_mask)
//{
//BlockLink_t pxFirstFreeBlock;
//BLOСK_OFFSET pucAlignedHeap;
//BLOСK_OFFSET ulAddress;
//BLOСK_SIZE	xTotalHeapSize;
//
//	byte_aligment=aligment;
//
//	xHeapStructSize	= ( sizeof( BLOСK_OFFSET ) + sizeof( BLOСK_SIZE ) + ( byte_aligment  -  1 ) ) & ~(byte_aligment-1);
//	minimum_block_size =  xHeapStructSize<<1;
//
//	/* Ensure the heap starts on a correctly aligned boundary. */
//	ulAddress = addr+xHeapStructSize;
//	xTotalHeapSize = size-xHeapStructSize;
//
//	if( ( ulAddress & (byte_aligment-1) ) != 0 )
//	{
//		ulAddress += byte_aligment-1 ;
//		ulAddress &=  ~( ( UINT32_T ) (byte_aligment-1) );
//		xTotalHeapSize -= ulAddress - addr;
//	}
//
//	pucAlignedHeap = ulAddress;
//
//	/* xStart is used to hold a pointer to the first item in the list of free
//	blocks.  The void cast is used to prevent compiler warnings. */
//	xStart.pxCurrentAddr=ulAddress-xHeapStructSize;
//	xStart.body.pxNextFreeBlock = pucAlignedHeap;
//	xStart.body.xBlockSize = 0;
//	sector_write(0,xStart.pxCurrentAddr,(void*)&xStart.body,xHeapStructSize,byte_aligment);
//
//	/* pxEnd is used to mark the end of the list of free blocks and is inserted
//	at the end of the heap space. */
//	ulAddress = ( ( UINT32_T ) pucAlignedHeap ) + xTotalHeapSize;
//	ulAddress -= xHeapStructSize;
//	ulAddress &= ~( ( UINT32_T ) (byte_aligment-1));
//	
//	pxEnd.body.pxNextFreeBlock=0;
//	pxEnd.body.xBlockSize=0;
//	pxEnd.pxCurrentAddr=ulAddress;
//	sector_write(0,pxEnd.pxCurrentAddr,(void*)&pxEnd.body, xHeapStructSize,byte_aligment);
//	
//	/* To start with there is a single free block that is sized to take up the
//	entire heap space, minus the space taken by pxEnd. */
//	pxFirstFreeBlock.pxCurrentAddr=pucAlignedHeap;
//	pxFirstFreeBlock.body.xBlockSize= ulAddress - pxFirstFreeBlock.pxCurrentAddr;
//	pxFirstFreeBlock.body.pxNextFreeBlock= pxEnd.pxCurrentAddr;
//	sector_write(0,pxFirstFreeBlock.pxCurrentAddr,(void*)&pxFirstFreeBlock.body, xHeapStructSize,byte_aligment);
//
//	/* Only one block exists - and it covers the entire usable heap space. */
//	xFreeBytesRemaining = pxFirstFreeBlock.body.xBlockSize;
//
//	xSegmentCounter=2;
//
//	/* Work out the position of the top bit in a size_t variable. */
//	xBlockAllocatedBit = ( ( BLOСK_SIZE ) 1 ) << ( ( sizeof( BLOСK_SIZE ) * heapBITS_PER_BYTE ) - 1 );
//
//	malloc_init_state=TRUE;
//}

/*-----------------------------------------------------------*/
//static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert )
//{
//BlockLink_t *pxIterator,pxIteratorBuf,pxBlockToInsertBuf;
//
//	/* Iterate through the list until a block is found that has a higher address
//	than the block being inserted. */
//	pxIterator = &xStart;
//
//	while(pxIterator->body.pxNextFreeBlock < pxBlockToInsert->pxCurrentAddr)
//	{
//		pxIteratorBuf.pxCurrentAddr=pxIterator->body.pxNextFreeBlock;
//		sector_read(0, pxIteratorBuf.pxCurrentAddr, (void*)&pxIteratorBuf.body, xHeapStructSize);
//		pxIterator = &pxIteratorBuf;
//	}
//
//	/* Do the block being inserted, and the block it is being inserted after
//	make a contiguous block of memory? */
//	if((pxIterator->pxCurrentAddr+pxIterator->body.xBlockSize)==pxBlockToInsert->pxCurrentAddr)
//	{
//		pxIterator->body.xBlockSize+=pxBlockToInsert->body.xBlockSize;
//		pxBlockToInsert = pxIterator;
//
//		xSegmentCounter--;
//	}
//
//	/* Do the block being inserted, and the block it is being inserted before
//	make a contiguous block of memory? */
//	if((pxBlockToInsert->pxCurrentAddr+pxBlockToInsert->body.xBlockSize)==pxIterator->body.pxNextFreeBlock)
//	{
//		if(pxIterator->body.pxNextFreeBlock!=pxEnd.pxCurrentAddr)
//		{
//			/* Form one big block from the two blocks. */
//			pxBlockToInsertBuf.pxCurrentAddr=pxIterator->body.pxNextFreeBlock;
//			sector_read(0, pxBlockToInsertBuf.pxCurrentAddr, (void*)&pxBlockToInsertBuf.body, xHeapStructSize);
//
//			pxBlockToInsert->body.xBlockSize=pxBlockToInsertBuf.body.xBlockSize;
//			pxBlockToInsert->body.pxNextFreeBlock=pxBlockToInsertBuf.body.pxNextFreeBlock;
//
//			xSegmentCounter--;
//		}
//		else
//		{
//			pxBlockToInsert->body.pxNextFreeBlock=pxEnd.pxCurrentAddr;
//		}
//	}
//	else
//	{
//		pxBlockToInsert->body.pxNextFreeBlock=pxIterator->body.pxNextFreeBlock;
//	}
//
//	/* If the block being inserted plugged a gab, so was merged with the block
//	before and the block after, then it's pxNextFreeBlock pointer will have
//	already been set, and should not be set here as that would make it point
//	to itself. */
//	if( pxIterator->pxCurrentAddr != pxBlockToInsert->pxCurrentAddr )
//	{
//		pxIterator->body.pxNextFreeBlock = pxBlockToInsert->pxCurrentAddr;
//		sector_write(0, pxIterator->pxCurrentAddr, (void*)&pxIterator->body, xHeapStructSize,byte_aligment);
//	}
//	sector_write(0,pxBlockToInsert->pxCurrentAddr,(void*)&pxBlockToInsert->body,xHeapStructSize,byte_aligment);
//}

/*-----------------------------------------------------------*/
//создание сектора
UINT8_T sector_Create(UINT8_T count)
{
	UINT16_T size;

	size=DB_HEADER_SIZE+SECTOR_SIZE*count;

	sl=(DataBase*)local_malloc(size); //размер структуры с описанием секторов
	xStart=(BlockLink*)local_malloc(sizeof(BlockLink));
	pxEnd=(BlockLink*)local_malloc(sizeof(BlockLink));
	

	if((sl==NULL) || (xStart==NULL) || (pxEnd==NULL))
	{
		local_free((void*)sl);
		local_free((void*)xStart);
		local_free((void*)pxEnd);

		return ERR_LOCAL_MALLOC;
	}

	memset((void*)sl,0x00,size);

	sl->sector_counter=count;

	ApplicationSectorPrepareHook();
	
	return ERR_OK;
}

//добавление описания новых секторов
UINT8_T sector_Insert(SectorConfig* config)
{
	UINT16_T size;

	if(sl!=NULL)
	{
		if(config->ByteAligment==0)
			return ERR_WRONG_ALIGMENT;

		if(config->index>(sl->sector_counter-1))
			return ERR_WRONG_INDEX;

		//вызвать хук, если требуется подготовить сектор к работе
		ApplicationSectorStartHook(config->index, config->StartAddr,config->SectorSize);

		sl->sector[config->index].StartAddrLen=config->StartAddrLen;
		sl->sector[config->index].SectorSizeLen=config->SectorSizeLen;
		sl->sector[config->index].Type=config->type;

		size=config->StartAddrLen + config->SectorSizeLen + (config->ByteAligment-1);
		if((config->type & SECTOR_CRC)>0)
			size+=sizeof(UINT16_T);

		size&=~(config->ByteAligment-1);

		sl->sector[config->index].bl_size=(UINT8_T)size;

		sl->sector[config->index].StartAddr=config->StartAddr;
		sl->sector[config->index].xStart_Addr=config->StartAddr;
		sl->sector[config->index].SectorSize=config->SectorSize;
		sl->sector[config->index].FreeBytesRemaining=config->SectorSize;
		sl->sector[config->index].ByteAligment=config->ByteAligment;

		//если сектор обычный ничего не коректировать
		if((config->type & SECTOR_MAIN)>0)
		{
			//если сектор SECTOR_MAIN только, то ничего не делаем.

			//с учетом выравнивания данного сектора!!!
			size=((DB_HEADER_SIZE+SECTOR_SIZE*sl->sector_counter)+( config->ByteAligment-1)) & ~(config->ByteAligment-1);

			//если сектор только SECTOR_MAIN, скоректировать FreeBytesRemaining
			if(config->SectorSize < size)
				return ERR_NO_FREE_SPACE;

			sl->sector[config->index].FreeBytesRemaining=config->SectorSize-size;

			if((config->type & SECTOR_START)>0)
			{
				//если сектор SECTOR_MAIN и SECTOR_START, скоректировать FreeBytesRemaining и xStart_Addr
				sl->sector[config->index].xStart_Addr+=size;
			}
			else
			{
				//если сектор SECTOR_MAIN только, то ничего не делаем.
				return ERR_OK;
			}
		}

		//если сектор SECTOR_MAIN и SECTOR_START, то инициализируем.
		//если сектор обычный, то инициализируем
		return sector_Init(config->index);
	}
	return ERR_LOCAL_MALLOC;
}


UINT8_T sector_Init(UINT8_T index)
{
	BlockLink		*pxFirstFreeBlock;
	UINT32_T		pucAlignedHeap;
	UINT32_T		ulAddress;
	UINT32_T		xTotalHeapSize;
	UINT8_T			err;

	pxFirstFreeBlock=(BlockLink*)local_malloc(sizeof(BlockLink));

	if(pxFirstFreeBlock==NULL)
		return ERR_LOCAL_MALLOC;

	/* Ensure the heap starts on a correctly aligned boundary. */
	ulAddress =	sl->sector[index].xStart_Addr + sl->sector[index].bl_size;
	xTotalHeapSize = sl->sector[index].FreeBytesRemaining - sl->sector[index].bl_size;

	if( ( ulAddress & (sl->sector[index].ByteAligment-1) ) != 0 )
	{
		ulAddress += sl->sector[index].ByteAligment-1 ;
		ulAddress &=  ~( ( UINT32_T ) (sl->sector[index].ByteAligment-1) );
		xTotalHeapSize -= ulAddress - sl->sector[index].xStart_Addr;
	}

	pucAlignedHeap = ulAddress;

	/* xStart is used to hold a pointer to the first item in the list of free
	blocks.  The void cast is used to prevent compiler warnings. */
	sl->sector[index].xStart_Addr= ulAddress - sl->sector[index].bl_size;
	xStart->pxCurrentAddr=sl->sector[index].xStart_Addr;
	xStart->body.pxNextFreeBlock=pucAlignedHeap;
	xStart->body.xBlockSize=0;

	err=WriteBlockLink(index,xStart);

	if(err==ERR_OK)
	{
		/* pxEnd is used to mark the end of the list of free blocks and is inserted
		at the end of the heap space. */
		ulAddress = ( ( UINT32_T ) pucAlignedHeap ) + xTotalHeapSize;
		ulAddress -= sl->sector[index].bl_size;
		ulAddress &= ~( (UINT32_T)(sl->sector[index].ByteAligment-1));

		pxEnd->body.pxNextFreeBlock=0;
		pxEnd->body.xBlockSize=0;
		sl->sector[index].pxEnd_Addr=ulAddress;
		pxEnd->pxCurrentAddr=sl->sector[index].pxEnd_Addr;
		err=WriteBlockLink(index,pxEnd);

		if(err==ERR_OK)
		{
			/* To start with there is a single free block that is sized to take up the
			entire heap space, minus the space taken by pxEnd. */
			pxFirstFreeBlock->pxCurrentAddr=pucAlignedHeap;
			pxFirstFreeBlock->body.xBlockSize= ulAddress - pucAlignedHeap;
			pxFirstFreeBlock->body.pxNextFreeBlock=sl->sector[index].pxEnd_Addr;

			err=WriteBlockLink(index,pxFirstFreeBlock);

			if(err==ERR_OK)
			{
				/* Only one block exists - and it covers the entire usable heap space. */
				sl->sector[index].FreeBytesRemaining = pxFirstFreeBlock->body.xBlockSize;

				sl->sector[index].xSegmentCounter=3;
			}
		}
	}

	local_free((void*)pxFirstFreeBlock);

	return err;
}

//запись не занятого заголовка сегмента
UINT8_T WriteBlockLink(UINT8_T index, BlockLink *bl)
{
	UINT16_T crc;
	UINT8_T	 *buf;
	UINT8_T	 i;

	buf=(UINT8_T*)local_malloc(sl->sector[index].bl_size);

	if(buf==NULL)
		return ERR_LOCAL_MALLOC;

	memset((void*)buf,0x00,sl->sector[index].bl_size);

	//сформировать пакет для записи и сгенерить crc16 если требуется
	//копируем заголовок сегмента в массив для записи
	i=0;
	memcpy((void*)buf,(void*)&bl->body.pxNextFreeBlock,sl->sector[index].StartAddrLen);
	i=sl->sector[index].StartAddrLen;
	memcpy((void*)(buf+i),(void*)&bl->body.xBlockSize,sl->sector[index].SectorSizeLen);
	i+=sl->sector[index].SectorSizeLen;

	if((sl->sector[index].Type & SECTOR_CRC)>0)
	{
		Crc16_Clear();
		crc=Crc16(buf,i);
		memcpy((void*)(buf+i),(void*)&crc,sizeof(UINT16_T));
	}

	i=ERR_OK;

	i=sector_write(index, bl->pxCurrentAddr, (void*)buf, sl->sector[index].bl_size);

	local_free(buf);

	return i;
}

UINT8_T ReadBlockLink(UINT8_T index , BlockLink *bl)
{
	UINT16_T crc;
	UINT8_T	 *buf;
	UINT8_T	 i;

	buf=(UINT8_T*)local_malloc(sl->sector[index].bl_size);

	if(buf==NULL)
		return ERR_LOCAL_MALLOC;

	i=sector_read(index, bl->pxCurrentAddr, (void*)buf, sl->sector[index].bl_size);

	if(i!=ERR_OK)
		return i;

	//проверка CRC если требуется
	if((sl->sector[index].Type & SECTOR_CRC)>0)
	{
		Crc16_Clear();
		i=sl->sector[index].StartAddrLen + sl->sector[index].SectorSizeLen; //размер структуры BlockLink
		crc=Crc16(buf,i);
		
		i=memcmp((void*)(buf+i),(void*)&crc,sizeof(UINT16_T));
		if(i!=0x00)
		{
			local_free(buf);
			return ERR_CRC;
		}
	}

	//загружаем в структуру
	i=sl->sector[index].StartAddrLen;
	memcpy((void*)&bl->body.pxNextFreeBlock,(void*)buf,i);
	memcpy((void*)&bl->body.xBlockSize,(void*)(buf+i),sl->sector[index].SectorSizeLen);
	
	return ERR_OK;
}


UINT8_T	sector_Malloc(UINT8_T index,UINT32_T *addr, UINT32_T xWantedSize )
{
	BlockLink *pxBlock, *pxNewBlockLink, *pxPreviousBlock;
	UINT8_T err=ERR_OK;

	pxBlock=(BlockLink*)local_malloc(sizeof(BlockLink));
	pxNewBlockLink=(BlockLink*)local_malloc(sizeof(BlockLink));

	if( (pxBlock==NULL) || (pxNewBlockLink==NULL))
	{
		err=ERR_LOCAL_MALLOC;
		goto exit;
	}

	memset((void*)pxBlock,0x00,sizeof(BlockLink));
	memset((void*)pxNewBlockLink,0x00,sizeof(BlockLink));

	/* The wanted size is increased so it can contain a BlockLink_t
	structure in addition to the requested amount of bytes. */
	if(xWantedSize==0)
	{
		err=ERR_WRONG_SIZE;
		goto exit;
	}

	xWantedSize += sl->sector[index].bl_size;

	/* Ensure that blocks are always aligned to the required number
	of bytes. */
	if((xWantedSize & (sl->sector[index].ByteAligment-1)) !=0x00)
	{
		/* Byte alignment required. */
		xWantedSize += (sl->sector[index].ByteAligment - (xWantedSize & (sl->sector[index].ByteAligment-1)));
	}

	if(xWantedSize > sl->sector[index].FreeBytesRemaining)
	{
		err=ERR_NO_FREE_SPACE;
		goto exit;
	}

	/* Traverse the list from the start	(lowest address) block until
	one	of adequate size is found. */
	//чтение сектора xStart
	xStart->pxCurrentAddr=sl->sector[index].xStart_Addr;
	err=ReadBlockLink(index, xStart);

	if(err!=ERR_OK)
		goto exit;

	pxPreviousBlock = xStart;

	pxBlock->pxCurrentAddr=xStart->body.pxNextFreeBlock;
	err=ReadBlockLink(index,pxBlock);

	if(err!=ERR_OK)
		goto exit;

	while( (pxBlock->body.xBlockSize < xWantedSize) && (pxBlock->body.pxNextFreeBlock !=0) )
	{
		pxPreviousBlock=pxBlock; //скопировать а не ссылка!

		pxBlock->pxCurrentAddr=pxBlock->body.pxNextFreeBlock;
		err=ReadBlockLink(index,pxBlock);

		if(err!=ERR_OK)
			goto exit;
	}

	/* If the end marker was reached then a block of adequate size
	was	not found. */
	if(pxBlock->pxCurrentAddr != sl->sector[index].pxEnd_Addr)
	{
		/* Return the memory space pointed to - jumping over the
		BlockLink_t structure at its start. */
		*addr=pxPreviousBlock->body.pxNextFreeBlock + sl->sector[index].bl_size;

		/* This block is being returned for use so must be taken out
		of the list of free blocks. */
		if(pxPreviousBlock->pxCurrentAddr != pxBlock->pxCurrentAddr)
		{
			pxPreviousBlock->body.pxNextFreeBlock=pxBlock->body.pxNextFreeBlock;

			err=WriteBlockLink(index,pxPreviousBlock);
			if(err!=ERR_OK)
				goto exit;
		}

		/* If the block is larger than required it can be split into
		two. */
		if( (pxBlock->body.xBlockSize - xWantedSize) > (UINT32_T)(sl->sector[index].bl_size<<1) )
		{
			/* This block is to be split into two.  Create a new
			block following the number of bytes requested. The void
			cast is used to prevent byte alignment warnings from the
			compiler. */
			pxNewBlockLink->pxCurrentAddr = pxBlock->pxCurrentAddr + xWantedSize;

			/* Calculate the sizes of two blocks split from the
			single block. */
			pxNewBlockLink->body.xBlockSize=pxBlock->body.xBlockSize - xWantedSize;
			pxNewBlockLink->body.pxNextFreeBlock=0;

			pxBlock->body.xBlockSize=xWantedSize;

			/* Insert the new block into the list of free blocks. */
			err=prvInsertBlockIntoFreeList(index , pxNewBlockLink);

			if(err!=ERR_OK)
				goto exit;

			sl->sector[index].xSegmentCounter++;
		}

		sl->sector[index].FreeBytesRemaining -= pxBlock->body.xBlockSize;

		/* The block is being returned - it is allocated and owned
		by the application and has no "next" block. */
		pxBlock->body.xBlockSize |= (UINT32_T)(0x01 << ((8*sl->sector[index].SectorSizeLen)-1));								//!!!!!!!!!!!!!!!!!!!!
		pxBlock->body.pxNextFreeBlock = 0;
		err=WriteBlockLink(index,pxBlock);
		//if(err!=ERR_OK)
		//	goto exit;
	}

exit:

	local_free((void*)pxBlock);
	local_free((void*)pxNewBlockLink);

	return err;
}


/*-----------------------------------------------------------*/
UINT8_T prvInsertBlockIntoFreeList(UINT8_T index, BlockLink *pxBlockToInsert )
{
	BlockLink *pxIterator,*pxBlockToInsertBuf;
	UINT8_T err=ERR_OK;

	pxBlockToInsertBuf=(BlockLink*)local_malloc(sizeof(BlockLink));

	if(pxBlockToInsertBuf==NULL)
	{
		err=ERR_LOCAL_MALLOC;
		goto exit;
	}

	memset((void*)pxBlockToInsertBuf,0x00,sizeof(BlockLink));

	/* Iterate through the list until a block is found that has a higher address
	than the block being inserted. */
	pxIterator = xStart;

	while(pxIterator->body.pxNextFreeBlock < pxBlockToInsert->pxCurrentAddr)
	{
		pxIterator->pxCurrentAddr=pxIterator->body.pxNextFreeBlock;
		err=ReadBlockLink(index,pxIterator);
		if(err!=ERR_OK)
			goto exit;
	}

	/* Do the block being inserted, and the block it is being inserted after
	make a contiguous block of memory? */
	if( (pxIterator->pxCurrentAddr + pxIterator->body.pxNextFreeBlock ) == pxBlockToInsert->pxCurrentAddr )
	{
		pxIterator->body.xBlockSize+=pxBlockToInsert->body.xBlockSize;
		pxBlockToInsert = pxIterator;

		sl->sector[index].xSegmentCounter--;
	}

	/* Do the block being inserted, and the block it is being inserted before
	make a contiguous block of memory? */
	if((pxBlockToInsert->pxCurrentAddr+pxBlockToInsert->body.xBlockSize)==pxIterator->body.pxNextFreeBlock)
	{
		if(pxIterator->body.pxNextFreeBlock!=pxEnd->pxCurrentAddr)
		{
			/* Form one big block from the two blocks. */
			pxBlockToInsertBuf->pxCurrentAddr=pxIterator->body.pxNextFreeBlock;
			err=ReadBlockLink(index,pxBlockToInsertBuf);
			if(err!=ERR_OK)
				goto exit;

			pxBlockToInsert->body.xBlockSize=pxBlockToInsertBuf->body.xBlockSize;
			pxBlockToInsert->body.pxNextFreeBlock=pxBlockToInsertBuf->body.pxNextFreeBlock;

			sl->sector[index].xSegmentCounter--;
		}
		else
		{
			pxBlockToInsert->body.pxNextFreeBlock=pxEnd->pxCurrentAddr;
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
		err=WriteBlockLink(index,pxIterator);
		if(err!=ERR_OK)
			goto exit;
	}

	err=WriteBlockLink(index,pxBlockToInsert);
	//if(err!=ERR_OK)
	//	goto exit;
exit:
	local_free((void*)pxBlockToInsertBuf);

	return ERR_OK;
}

/*-----------------------------------------------------------*/
UINT8_T	sector_Free(UINT8_T index, UINT32_T pv )
{
	UINT32_T puc=pv;
	BlockLink *pxLink;
	UINT8_T err=ERR_OK;

	pxLink=(BlockLink*)local_malloc(sizeof(BlockLink));

	if(pxLink==NULL)
	{
		err=ERR_LOCAL_MALLOC;
		goto exit;
	}

	memset((void*)pxLink,0x00,sizeof(BlockLink));

	if( pv != 0 )
	{
		/* The memory being freed will have an BlockLink_t structure immediately
		before it. */
		puc -= sl->sector[index].bl_size;

		/* This casting is to keep the compiler from issuing warnings. */
		pxLink->pxCurrentAddr=puc;
		err=ReadBlockLink(index,pxLink);
		if(err!=ERR_OK)
			goto exit;

		if( ( pxLink->body.xBlockSize & (UINT32_T)(0x01 << ((8*sl->sector[index].SectorSizeLen)-1)) ) != 0 )								//!!!!!!!!!!!!!!
		{
			if( pxLink->body.pxNextFreeBlock == 0 )
			{
				/* The block is being returned to the heap - it is no longer
				allocated. */
				pxLink->body.xBlockSize&=~(UINT32_T)(0x01 << ((8*sl->sector[index].SectorSizeLen)-1));

				/* Add this block to the list of free blocks. */
				sl->sector[index].FreeBytesRemaining+=pxLink->body.xBlockSize;
				prvInsertBlockIntoFreeList(index,pxLink);
			}
		}
	}

exit:
	local_free((void*)pxLink);

	return err;
}

////удалить сектор
//void sector_Delete()
//{
//}
//
////открыть
//void sector_Open()
//{
//}
//
////закрыть бд
//void sector_Close()
//{
//}

