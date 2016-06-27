#include <stdlib.h>
#include <memory.h>
#include "heap.h"
#include "sql_db_config.h"

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
UINT8_T prvInsertBlockIntoFreeList(UINT8_T index, BlockLink *pxBlockToInsert );

extern void ApplicationSectorPrepareHook( void ); //���������� ���� ��������
extern void ApplicationSectorStartHook(UINT8_T index, UINT32_T start_addr,UINT32_T size); //���������� �������

/*-----------------------------------------------------------*/

//---------------------------- NEW -------------------------------------------

UINT8_T WriteBlockLink(UINT8_T index, BlockLink *bl);
UINT8_T ReadBlockLink(UINT8_T index, BlockLink *bl);

DataBase *sl=NULL;	//�������� �������� ���� ������
BlockLink *xStart;	//��������� ���������

/*-----------------------------------------------------------*/
UINT32_T sector_GetFreeSize(UINT8_T index)
{
	return sl->sector[index].FreeBytesRemaining;
}

/*-----------------------------------------------------------*/
#if (configUSE_SegmentCounter==TRUE)
UINT32_T sector_GetSegmentCounter(UINT8_T index)
{
	return sl->sector[index].xSegmentCounter;
}
#endif

/*-----------------------------------------------------------*/
//�������� �������
UINT8_T sector_Create(UINT8_T count)
{
	UINT16_T size;

	size=DB_HEADER_SIZE+SECTOR_SIZE*count;

	sl=(DataBase*)local_malloc(size); //������ ��������� � ��������� ��������
	xStart=(BlockLink*)local_malloc(sizeof(BlockLink));

	if((sl==NULL) || (xStart==NULL) )
	{
		local_free((void*)sl);
		local_free((void*)xStart);

		return ERR_LOCAL_MALLOC;
	}

	memset((void*)sl,0x00,size);

	sl->sector_counter=count;

	ApplicationSectorPrepareHook();
	
	return ERR_OK;
}

/*-----------------------------------------------------------*/
UINT8_T	sector_ConfigCheck(SectorConfig* config)
{
	if(config->ByteAligment==0)
		return ERR_WRONG_ALIGMENT;

	if(config->SectorSize==0)
		return ERR_NO_FREE_SPACE;

	if(config->SectorSizeLen==0)
		return ERR_ADDR_LEN;

	if(config->StartAddrLen==0)
		return ERR_SIZE_LEN;

	if( (config->type & SECTOR_MAIN) >0 )
	{
		if((config->type & SECTOR_READONLY) >0 )
			return ERR_MAIN_READONLY;

		if((config->type & 0xF0) ==0)
			return ERR_MAIN_SECTOR_FREE;
	}

	if( (config->type & SECTOR_START) >0 )
	{
		if( (config->type & SECTOR_FREE) > 0)
			return ERR_START_SECTOR_FREE;
	}
	
	//������������ ������
	config->StartAddr+=(config->ByteAligment - 1);
	config->StartAddr&=~(config->ByteAligment - 1);

	//������������ �������
	config->SectorSize&=~(config->ByteAligment - 1);

	return ERR_OK;
}

UINT8_T sector_GetSectorConfig(UINT8_T index, SectorConfig* config)
{
	if(sl!=NULL)
	{
		if(index > (sl->sector_counter-1))
			return ERR_WRONG_INDEX;
		
		config->index=index;
		config->type=sl->sector[index].Type;
		config->ByteAligment=sl->sector[index].ByteAligment;
		config->StartAddr=sl->sector[index].StartAddr;
		config->StartAddrLen=sl->sector[index].StartAddrLen;
		config->SectorSize=(sl->sector[index].pxEnd_Addr+sl->sector[index].bl_size)-sl->sector[index].StartAddr;
		config->SectorSizeLen=sl->sector[index].SectorSizeLen;

		return ERR_OK;
	}

	return ERR_LOCAL_MALLOC;
}

//���������� �������� ����� ��������
UINT8_T sector_Insert(SectorConfig* config)
{
	UINT16_T size;

	if(sl!=NULL)
	{
		if(config->index > (sl->sector_counter-1))
			return ERR_WRONG_INDEX;

		//������� ���, ���� ��������� ����������� ������ � ������
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
		sl->sector[config->index].FreeBytesRemaining=config->SectorSize;
		sl->sector[config->index].ByteAligment=config->ByteAligment;

		//���� ������ ������� ������ �� �������������
		if((config->type & SECTOR_MAIN)>0)
		{
			//���� ������ SECTOR_MAIN ������, �� ������ �� ������.

			//� ������ ������������ ������� �������!!!
			size=((DB_HEADER_SIZE+SECTOR_SIZE*sl->sector_counter)+( config->ByteAligment-1)) & ~(config->ByteAligment-1);

			//���� ������ ������ SECTOR_MAIN, �������������� FreeBytesRemaining
			if(config->SectorSize < size)
				return ERR_NO_FREE_SPACE;

			sl->sector[config->index].FreeBytesRemaining=config->SectorSize-size;

			if((config->type & SECTOR_START)>0)
			{
				//���� ������ SECTOR_MAIN � SECTOR_START, �������������� FreeBytesRemaining � xStart_Addr
				sl->sector[config->index].xStart_Addr+=size;
			}
			else
			{
				//���� ������ SECTOR_MAIN ������, �� ������ �� ������.
				return ERR_OK;
			}
		}

		//���� ������ SECTOR_MAIN � SECTOR_START, �� ��������������.
		//���� ������ �������, �� ��������������
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

		//�������� pxEnd ��������� ������ pxFirstFreeBlock
		pxFirstFreeBlock->body.pxNextFreeBlock=0;
		pxFirstFreeBlock->body.xBlockSize=0;
		sl->sector[index].pxEnd_Addr=ulAddress;
		pxFirstFreeBlock->pxCurrentAddr=sl->sector[index].pxEnd_Addr;
		err=WriteBlockLink(index,pxFirstFreeBlock);

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

				#if (configUSE_SegmentCounter==TRUE)
				sl->sector[index].xSegmentCounter=3;
				#endif
			}
		}
	}

	local_free((void*)pxFirstFreeBlock);

	return err;
}

//������ �� �������� ��������� ��������
UINT8_T WriteBlockLink(UINT8_T index, BlockLink *bl)
{
	UINT16_T crc;
	UINT8_T	 *buf;
	UINT8_T	 i;

	buf=(UINT8_T*)local_malloc(sl->sector[index].bl_size);

	if(buf==NULL)
		return ERR_LOCAL_MALLOC;

	memset((void*)buf,0x00,sl->sector[index].bl_size);

	//������������ ����� ��� ������ � ��������� crc16 ���� ���������
	//�������� ��������� �������� � ������ ��� ������
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

	//�������� CRC ���� ���������
	if((sl->sector[index].Type & SECTOR_CRC)>0)
	{
		Crc16_Clear();
		i=sl->sector[index].StartAddrLen + sl->sector[index].SectorSizeLen; //������ ��������� BlockLink
		crc=Crc16(buf,i);
		
		i=memcmp((void*)(buf+i),(void*)&crc,sizeof(UINT16_T));
		if(i!=0x00)
		{
			local_free(buf);
			return ERR_CRC;
		}
	}

	//��������� � ���������
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
	//������ ������� xStart
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
		pxPreviousBlock=pxBlock; //����������� � �� ������!

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

			#if (configUSE_SegmentCounter==TRUE)
			sl->sector[index].xSegmentCounter++;
			#endif
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

	//������ ������� xStart
	xStart->pxCurrentAddr=sl->sector[index].xStart_Addr;
	err=ReadBlockLink(index, xStart);
	if(err!=ERR_OK)
		goto exit;

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
	if( (pxIterator->pxCurrentAddr + pxIterator->body.xBlockSize ) == pxBlockToInsert->pxCurrentAddr )
	{
		pxIterator->body.xBlockSize+=pxBlockToInsert->body.xBlockSize;
		pxBlockToInsert = pxIterator;

		#if (configUSE_SegmentCounter==TRUE)
		sl->sector[index].xSegmentCounter--;
		#endif
	}

	/* Do the block being inserted, and the block it is being inserted before
	make a contiguous block of memory? */
	if((pxBlockToInsert->pxCurrentAddr+pxBlockToInsert->body.xBlockSize)==pxIterator->body.pxNextFreeBlock)
	{
		//if(pxIterator->body.pxNextFreeBlock!=pxEnd->pxCurrentAddr) //������!
		if(pxIterator->body.pxNextFreeBlock!=sl->sector[index].pxEnd_Addr)
		{
			/* Form one big block from the two blocks. */
			pxBlockToInsertBuf->pxCurrentAddr=pxIterator->body.pxNextFreeBlock;
			err=ReadBlockLink(index,pxBlockToInsertBuf);
			if(err!=ERR_OK)
				goto exit;

			pxBlockToInsert->body.xBlockSize=pxBlockToInsertBuf->body.xBlockSize;
			pxBlockToInsert->body.pxNextFreeBlock=pxBlockToInsertBuf->body.pxNextFreeBlock;

			#if (configUSE_SegmentCounter==TRUE)
			sl->sector[index].xSegmentCounter--;
			#endif
		}
		else
		{
			pxBlockToInsert->body.pxNextFreeBlock=sl->sector[index].pxEnd_Addr;//pxEnd->pxCurrentAddr;
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

void sector_ResourceFree()
{
	local_free((void*)sl);
	local_free((void*)xStart);
}

////������� ������
//void sector_Delete()
//{
//}
//
////�������
//void sector_Open()
//{
//}
//
////������� ��
//void sector_Close()
//{
//}

