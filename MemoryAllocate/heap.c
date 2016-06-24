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


//---------------------------- NEW -------------------------------------------

UINT8_T WriteBlockLink(UINT8_T index, UINT32_T addr, BlockLink_tt *bl);
UINT8_T ReadBlockLink(UINT8_T index, UINT32_T addr, BlockLink_tt *bl);

DataBase *db;							//описание секторов базы данных
static BlockLink_tt xStart1, pxEnd1;	//заголовки сегментов
//UINT8_T *BlockLink_buff;//[BLOCK_LINK_SIZE+sizeof(UINT16_T)]; //размер структуры + crc

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

	db=(DataBase*)local_malloc(size); //размер структуры с описанием секторов
	
	if(db==NULL)
		return ERR_LOCAL_MALLOC;

	//BlockLink_buff=(UINT8_T*)local_malloc((BLOCK_LINK_SIZE+sizeof(UINT16_T)));

	//if(BlockLink_buff==NULL)
	//	return ERR_LOCAL_MALLOC;

	memset((void*)db,0x00,size);
	db->sector_counter=count;
	ApplicationSectorPrepareHook();
	
	return ERR_OK;
}

UINT8_T sector_Insert(SectorConfig* config)
{
	UINT16_T size;

	if(db!=NULL)
	{
		if(config->ByteAligment==0)
			return ERR_WRONG_ALIGMENT;

		if(config->index>(db->sector_counter-1))
			return ERR_WRONG_INDEX;

		//вызвать хук, если требуется подготовить сектор к работе
		ApplicationSectorStartHook(config->index, config->StartAddr,config->SectorSize);

		//db->sector[config->index].FieldSize=(((UINT32_T)config->StartAddrLen)<<12) | (((UINT32_T)config->SectorSizeLen)<<8) | ((UINT32_T)config->type);
		db->sector[config->index].StartAddrLen=config->StartAddrLen;
		db->sector[config->index].SectorSizeLen=config->SectorSizeLen;
		db->sector[config->index].Type=config->type;

		size=config->StartAddrLen + config->SectorSizeLen + (config->ByteAligment-1);
		if((config->type & SECTOR_CRC)>0)
			size+=sizeof(UINT16_T);

		size&=~(config->ByteAligment-1);

		db->sector[config->index].bl_size=(UINT8_T)size;//(size) & ~(config->ByteAligment-1);

		db->sector[config->index].StartAddr=config->StartAddr;
		db->sector[config->index].xStart_Addr=config->StartAddr;
		db->sector[config->index].SectorSize=config->SectorSize;
		db->sector[config->index].FreeBytesRemaining=config->SectorSize;
		db->sector[config->index].ByteAligment=config->ByteAligment;

		//если сектор обычный ничего не коректировать
		if((config->type & SECTOR_MAIN)>0)
		{
			//если сектор SECTOR_MAIN только, то ничего не делаем.

			//с учетом выравнивания данного сектора!!!
			size=((DB_HEADER_SIZE+SECTOR_SIZE*db->sector_counter)+( config->ByteAligment-1)) & ~(config->ByteAligment-1);

			//если сектор только SECTOR_MAIN, скоректировать FreeBytesRemaining
			if(config->SectorSize<size)
				return ERR_NO_FREE_SPACE;

			db->sector[config->index].FreeBytesRemaining=config->SectorSize-size;

			if((config->type & SECTOR_START)>0)
			{
				//если сектор SECTOR_MAIN и SECTOR_START, скоректировать FreeBytesRemaining и xStart_Addr
				db->sector[config->index].xStart_Addr+=size;

				//если сектор SECTOR_MAIN и SECTOR_START, то инициализируем.
				//sector_Init1(config->index);
			}
			else
			{
				//если сектор SECTOR_MAIN только, то ничего не делаем.
				return ERR_OK;
			}
		}
		//else
		//{
		//	//если сектор обычный, то инициализируем
		//	sector_Init1(config->index);
		//}

		//return ERR_OK;

		//если сектор SECTOR_MAIN и SECTOR_START, то инициализируем.
		//если сектор обычный, то инициализируем
		return sector_Init1(config->index);
	}
	return ERR_LOCAL_MALLOC;
}





UINT8_T sector_Init1(UINT8_T index)
{
	//UINT32_T		FirstFreeBlock_Addr;
	BlockLink_tt	pxFirstFreeBlock;
	UINT32_T		pucAlignedHeap;
	UINT32_T		ulAddress;
	UINT32_T		xTotalHeapSize;
	UINT8_T			err;
//UINT16_T		crc;

//db->sector[index].ByteAligment
//	byte_aligment=aligment;

//	xHeapStructSize	= ( sizeof( BLOСK_OFFSET ) + sizeof( BLOСK_SIZE ) + ( byte_aligment  -  1 ) ) & ~(byte_aligment-1);
//	minimum_block_size =  xHeapStructSize<<1;

	/* Ensure the heap starts on a correctly aligned boundary. */
//	ulAddress = addr+xHeapStructSize;
	ulAddress =	db->sector[index].xStart_Addr + db->sector[index].bl_size;
//	xTotalHeapSize = size-xHeapStructSize;
	xTotalHeapSize = db->sector[index].FreeBytesRemaining - db->sector[index].bl_size;

//	if( ( ulAddress & (byte_aligment-1) ) != 0 )
//	{
//		ulAddress += byte_aligment-1 ;
//		ulAddress &=  ~( ( UINT32_T ) (byte_aligment-1) );
//		xTotalHeapSize -= ulAddress - addr;
//	}

	if( ( ulAddress & (db->sector[index].ByteAligment-1) ) != 0 )
	{
		ulAddress += db->sector[index].ByteAligment-1 ;
		ulAddress &=  ~( ( UINT32_T ) (db->sector[index].ByteAligment-1) );
		xTotalHeapSize -= ulAddress - db->sector[index].xStart_Addr;
	}


	pucAlignedHeap = ulAddress;

	/* xStart is used to hold a pointer to the first item in the list of free
	blocks.  The void cast is used to prevent compiler warnings. */
//	xStart.pxCurrentAddr=ulAddress-xHeapStructSize;
	db->sector[index].xStart_Addr= ulAddress - db->sector[index].bl_size;
//	xStart.body.pxNextFreeBlock = pucAlignedHeap;
	xStart1.pxNextFreeBlock=pucAlignedHeap;
//	xStart.body.xBlockSize = 0;
	xStart1.xBlockSize=0;

	//сформировать пакет для записи и сгенерить crc16 если требуется
	//копируем заголовок сегмента в массив для записи
	//memcpy((void*)(BlockLink_buff+sizeof(UINT16_T)),(void*)&xStart1.pxNextFreeBlock,db->sector[index].StartAddrLen);
	//memcpy((void*)(BlockLink_buff+sizeof(UINT16_T)+db->sector[index].StartAddrLen),(void*)&xStart1.xBlockSize,db->sector[index].SectorSizeLen);
	//if((db->sector[index].Type & SECTOR_CRC)>0)
	//{
	//	crc=Crc16((BlockLink_buff+sizeof(UINT16_T)), db->sector[index].StartAddrLen+db->sector[index].SectorSizeLen);
	//	memcpy((void*)(BlockLink_buff),(void*)&crc,sizeof(UINT16_T));
	//}
//	sector_write(0,xStart.pxCurrentAddr,(void*)&xStart.body,xHeapStructSize,byte_aligment);
	err=WriteBlockLink(index, db->sector[index].xStart_Addr,&xStart1);

	if(err!=ERR_OK)
		return err;

	/* pxEnd is used to mark the end of the list of free blocks and is inserted
	at the end of the heap space. */
	ulAddress = ( ( UINT32_T ) pucAlignedHeap ) + xTotalHeapSize;
	//ulAddress -= xHeapStructSize;
	ulAddress -= db->sector[index].bl_size;
//	ulAddress &= ~( ( UINT32_T ) (byte_aligment-1));
	ulAddress &= ~( (UINT32_T)(db->sector[index].ByteAligment-1));

//	pxEnd.body.pxNextFreeBlock=0;
	pxEnd1.pxNextFreeBlock=0;
//	pxEnd.body.xBlockSize=0;
	pxEnd1.xBlockSize=0;
//	pxEnd.pxCurrentAddr=ulAddress;
	db->sector[index].pxEnd_Addr=ulAddress;
//	sector_write(0,pxEnd.pxCurrentAddr,(void*)&pxEnd.body, xHeapStructSize,byte_aligment);
	err=WriteBlockLink(index, db->sector[index].pxEnd_Addr,&pxEnd1);

	if(err!=ERR_OK)
		return err;

	/* To start with there is a single free block that is sized to take up the
	entire heap space, minus the space taken by pxEnd. */
//	pxFirstFreeBlock.pxCurrentAddr=pucAlignedHeap;
//	pxFirstFreeBlock.body.xBlockSize= ulAddress - pxFirstFreeBlock.pxCurrentAddr;
	pxFirstFreeBlock.xBlockSize= ulAddress - pucAlignedHeap;
//	pxFirstFreeBlock.body.pxNextFreeBlock= pxEnd.pxCurrentAddr;
	pxFirstFreeBlock.pxNextFreeBlock= db->sector[index].pxEnd_Addr;
//	sector_write(0,pxFirstFreeBlock.pxCurrentAddr,(void*)&pxFirstFreeBlock.body, xHeapStructSize,byte_aligment);
	err=WriteBlockLink(index, pucAlignedHeap ,&pxFirstFreeBlock);

	if(err!=ERR_OK)
		return err;

	/* Only one block exists - and it covers the entire usable heap space. */
//	xFreeBytesRemaining = pxFirstFreeBlock.body.xBlockSize;
	db->sector[index].FreeBytesRemaining = pxFirstFreeBlock.xBlockSize;

//	xSegmentCounter=2;
	db->sector[index].xSegmentCounter=3;

	/* Work out the position of the top bit in a size_t variable. */
//	xBlockAllocatedBit = ( ( BLOСK_SIZE ) 1 ) << ( ( sizeof( BLOСK_SIZE ) * heapBITS_PER_BYTE ) - 1 );
//
//	malloc_init_state=TRUE;

	return ERR_OK;
}

//запись не занятого заголовка сегмента
UINT8_T WriteBlockLink(UINT8_T index, UINT32_T addr, BlockLink_tt *bl)
{
	UINT16_T crc;
	UINT8_T	 *buf;
	UINT8_T	 i;

	buf=(UINT8_T*)local_malloc(db->sector[index].bl_size);

	if(buf==NULL)
		return ERR_LOCAL_MALLOC;

	memset((void*)buf,0x00,db->sector[index].bl_size);

	//сформировать пакет для записи и сгенерить crc16 если требуется
	//копируем заголовок сегмента в массив для записи
	i=0;
	memcpy((void*)buf,(void*)&bl->pxNextFreeBlock,db->sector[index].StartAddrLen);
	i=db->sector[index].StartAddrLen;
	memcpy((void*)(buf+i),(void*)&bl->xBlockSize,db->sector[index].SectorSizeLen);
	i+=db->sector[index].SectorSizeLen;

	if((db->sector[index].Type & SECTOR_CRC)>0)
	{
		Crc16_Clear();
		crc=Crc16(buf,i);
		memcpy((void*)(buf+i),(void*)&crc,sizeof(UINT16_T));
	}

	i=ERR_OK;
	i=_sector_write(index, addr, (void*)buf, db->sector[index].bl_size);
	local_free(buf);

	return i;
}

UINT8_T ReadBlockLink(UINT8_T index, UINT32_T addr, BlockLink_tt *bl)
{
	UINT16_T crc;
	UINT8_T	 *buf;
	UINT8_T	 i;

	buf=(UINT8_T*)local_malloc(db->sector[index].bl_size);

	if(buf==NULL)
		return ERR_LOCAL_MALLOC;

	i=_sector_read(index, addr, (void*)buf, db->sector[index].bl_size);

	if(i!=ERR_OK)
		return i;

	//проверка CRC если требуется
	if((db->sector[index].Type & SECTOR_CRC)>0)
	{
		Crc16_Clear();
		i=db->sector[index].StartAddrLen + db->sector[index].SectorSizeLen; //размер структуры BlockLink
		crc=Crc16(buf,i);
		
		i=memcmp((void*)(buf+i),(void*)&crc,sizeof(UINT16_T));
		if(i!=0x00)
		{
			local_free(buf);
			return ERR_CRC;
		}
	}

	//загружаем в структуру
	i=db->sector[index].StartAddrLen;
	memcpy((void*)&bl->pxNextFreeBlock,(void*)buf,i);
	memcpy((void*)&bl->xBlockSize,(void*)(buf+i),db->sector[index].SectorSizeLen);
	
	return ERR_OK;
}


UINT8_T	sector_Malloc1(UINT8_T index,UINT32_T *addr, UINT32_T xWantedSize )
{
	BlockLink_tt pxBlock, *pxPreviousBlock, pxNewBlockLink;
	UINT32_T pxPreviousBlock_addr;
	UINT32_T pxNewBlockLink_addr;
	UINT32_T pxBlock_addr;
	UINT32_T pvReturn = 0;
	UINT8_T err;

	/* If this is the first call to malloc then the heap will require
	initialisation to setup the list of free blocks. */

	/* The wanted size is increased so it can contain a BlockLink_t
	structure in addition to the requested amount of bytes. */
	if(xWantedSize==0)
		return ERR_WRONG_SIZE;

	//if( xWantedSize > 0 )
	//{
	//	xWantedSize += xHeapStructSize;
	xWantedSize += db->sector[index].bl_size;

	/* Ensure that blocks are always aligned to the required number
	of bytes. */
	
	//	if( ( xWantedSize & (byte_aligment-1) ) != 0x00 )
	//	{
	//		/* Byte alignment required. */
	//		xWantedSize += ( byte_aligment - ( xWantedSize & (byte_aligment-1) ) ) ;
	//	}

	if((xWantedSize & (db->sector[index].ByteAligment-1)) !=0x00)
	{
		/* Byte alignment required. */
		xWantedSize += (db->sector[index].ByteAligment - (xWantedSize & (db->sector[index].ByteAligment-1)));
	}

	//}

	//if( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining ) )
	//{

	if(xWantedSize > db->sector[index].FreeBytesRemaining)
		return ERR_NO_FREE_SPACE;

	/* Traverse the list from the start	(lowest address) block until
	one	of adequate size is found. */
	//	pxPreviousBlock = &xStart;
	
	//чтение сектора xStart
	err=ReadBlockLink(index, db->sector[index].xStart_Addr,&xStart1);

	if(err!=ERR_OK)
		return err;

	pxPreviousBlock_addr=db->sector[index].xStart_Addr;
	pxPreviousBlock = &xStart1;

	//	sector_read(0,xStart.body.pxNextFreeBlock,(void*)&pxBlock.body,xHeapStructSize);
	err=ReadBlockLink(index,xStart1.pxNextFreeBlock,&pxBlock);
	//	pxBlock.pxCurrentAddr=xStart.body.pxNextFreeBlock;
	pxBlock_addr=xStart1.pxNextFreeBlock;

	//	while((pxBlock.body.xBlockSize<xWantedSize)&&(pxBlock.body.pxNextFreeBlock!=0))
	//	{
	//		pxPreviousBlock = &pxBlock;

	//		pxBlock.pxCurrentAddr=pxBlock.body.pxNextFreeBlock;
	//		sector_read(0,pxBlock.pxCurrentAddr,(void*)&pxBlock.body,xHeapStructSize);;
	//	}

	while((pxBlock.xBlockSize<xWantedSize)&&(pxBlock.pxNextFreeBlock!=0))
	{
		pxPreviousBlock_addr=pxBlock_addr;
		pxPreviousBlock = &pxBlock;

		pxBlock_addr=pxBlock.pxNextFreeBlock;
		err=ReadBlockLink(index,pxBlock_addr,&pxBlock);

		if(err!=ERR_OK)
			return err;
	}

	/* If the end marker was reached then a block of adequate size
	was	not found. */
	
	//err=ReadBlockLink(index,db->sector[index].pxEnd_Addr,&pxEnd1);

	//	if(pxBlock.pxCurrentAddr!=pxEnd.pxCurrentAddr)
	//	{

	if(pxBlock_addr!=db->sector[index].pxEnd_Addr)
	{

		/* Return the memory space pointed to - jumping over the
		BlockLink_t structure at its start. */
		//pvReturn = pxPreviousBlock->body.pxNextFreeBlock+xHeapStructSize;
		*addr=pxPreviousBlock->pxNextFreeBlock + db->sector[index].bl_size;

		/* This block is being returned for use so must be taken out
		of the list of free blocks. */
	//		pxPreviousBlock->body.pxNextFreeBlock=pxBlock.body.pxNextFreeBlock;
		pxPreviousBlock->pxNextFreeBlock=pxBlock.pxNextFreeBlock;
	//		sector_write(0,pxPreviousBlock->pxCurrentAddr,(void*)&pxPreviousBlock->body,xHeapStructSize,byte_aligment);
		err=WriteBlockLink(index,pxPreviousBlock_addr,pxPreviousBlock);

		if(err!=ERR_OK)
			return err;

		/* If the block is larger than required it can be split into
		two. */
		
	//		if( ( pxBlock.body.xBlockSize - xWantedSize ) > minimum_block_size)		
	//		{
		if( (pxBlock.xBlockSize-xWantedSize) > (UINT32_T)(db->sector[index].bl_size<<1) )
		{
			/* This block is to be split into two.  Create a new
			block following the number of bytes requested. The void
			cast is used to prevent byte alignment warnings from the
			compiler. */
	//			pxNewBlockLink.pxCurrentAddr=pxBlock.pxCurrentAddr+xWantedSize;
			pxNewBlockLink_addr=pxBlock_addr+xWantedSize;

			/* Calculate the sizes of two blocks split from the
			single block. */
	//			pxNewBlockLink.body.xBlockSize=pxBlock.body.xBlockSize-xWantedSize;
			pxNewBlockLink.xBlockSize=pxBlock.xBlockSize-xWantedSize;
	//			pxNewBlockLink.body.pxNextFreeBlock=0;
			pxNewBlockLink.pxNextFreeBlock=0;

	//			pxBlock.body.xBlockSize=xWantedSize;
			pxBlock.xBlockSize=xWantedSize;
	//			//writeMalloc(0, 0, pxBlock.pxCurrentAddr, (void*)&pxBlock.body,xHeapStructSize);
			
			//выяснить почему не записывать //блок записывается перед выходом
			//err=WriteBlockLink(index,pxBlock_addr,&pxBlock);
			//if(err!=ERR_OK)
			//	return err;

			/* Insert the new block into the list of free blocks. */
	//			prvInsertBlockIntoFreeList(&pxNewBlockLink);


	//		xSegmentCounter++;
			db->sector[index].xSegmentCounter++;
		}

	//		xFreeBytesRemaining -= pxBlock.body.xBlockSize;

	//		/* The block is being returned - it is allocated and owned
	//		by the application and has no "next" block. */
	//		pxBlock.body.xBlockSize |= xBlockAllocatedBit;									//!!!!!!!!!!!!!!!!!!!!
	//		pxBlock.body.pxNextFreeBlock = 0;
	//		sector_write(0, pxBlock.pxCurrentAddr, (void*)&pxBlock.body,xHeapStructSize,byte_aligment);
	}

	#if( configUSE_MALLOC_FAILED_HOOK == 1 )
	{
	//	if( pvReturn == NULL )
	//	{
	//		extern void vApplicationMallocFailedHook( void );
	//		vApplicationMallocFailedHook();
	//	}
	}
	#endif

	//return pvReturn;
	return ERR_OK;
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

