/**
 * @file spi_flash_MemMang.c
 * @author Donocean (1184427366@qq.com)
 * @brief 轻量化的用于 flash 的内存管理
 * @version 0.1
 * @date 2022-02-24
 * 
 * @copyright Copyright (c) 2022, Donocean
 */

#include "flash_MemMang.h"

/* 每个块状态，用于块交换时使用 */
#define REGION_ERASING       ((ee_uint32)0xFFFFFFFF)
#define REGION_VERIFIED     ((ee_uint32)0x00FFFFFF)
#define REGION_COPY         ((ee_uint32)0x0000FFFF)
#define REGION_ACTIVE       ((ee_uint32)0x000000FF)

/* 数据状态 */
#define DATA_EMPTY         ((ee_uint16)0xFFFF)
#define DATA_INVALID       ((ee_uint16)0x00FF)
#define DATA_HALFVALID     ((ee_uint16)0x000F)
#define DATA_VALID         ((ee_uint16)0x0000)

/* 数据索引结构 */
typedef struct 
{
	/* 当前数据状态 */
	ee_uint16 dataStatus;
	/* 当前数据大小 */
	ee_uint16 dataSize;
	/* 当前数据在数据区的地址(相对于dataStartAddr的偏移地址) */
	ee_uint16 dataAddr;
	/* 当前数据被重写的地址(相对于overwriteAddr的偏移地址)，默认为0xFFFF */
	ee_uint16 dataOverwriteAddr;
}ee_dataIndex;

static void flashMemMangHandle_Init(flash_MemMang_t* pobj,ee_uint32 indexStartAddr,ee_uint32 indexSwapStartAddr,ee_uint16 indexRegionSize,ee_uint16 indexSize,ee_uint32 dataStartAddr,ee_uint32 dataSwapStartAddr,ee_uint16 dataRegionSize);
static void countAreaPlusOne(flash_MemMang_t* pobj);
static ee_uint32 getFreeAddrInDataRegion(flash_MemMang_t* pobj);
static ee_uint32 getFreeAddrInOverwriteArea(flash_MemMang_t* pobj);
static void writeIndexAndData(flash_MemMang_t* pobj, ee_uint32 writeDataAddr, ee_uint32 writeIndexAddr, void* buf, ee_uint16 bufSize);
static ee_uint32 getLastIndexAddrThatNotBeenOverwritten(flash_MemMang_t* pobj, variableLists dataId);
static void eraseRegion(flash_MemMang_t *pobj, ee_uint32 regionAddr);
static ee_uint8 verifyRegionFullyErased(flash_MemMang_t *pobj, ee_uint32 regionAddr);
static void swapRegion(flash_MemMang_t* pobj);


// flash管理指针
// 索引区起始地址
// 索引交换区起始地址
// 索引区总大小
// 索引区大小(用于定位重写区地址) 要小于indexRegionSize
// 数据区起始地址
// 数据交换区起始地址
// 数据区总大小(单位：扇区)
void ee_flashInit(flash_MemMang_t* pobj,       \
                  ee_uint32 indexStartAddr,    \
				  ee_uint32 indexSwapStartAddr,\
				  ee_uint16 indexRegionSize,   \
				  ee_uint16 indexSize,         \
				  ee_uint32 dataStartAddr,     \
				  ee_uint32 dataSwapStartAddr, \
				  ee_uint16 dataRegionSize)         
{
	ee_uint32 regionStatus = 0;
	ee_uint32 swapRegionStatus = 0;

	/* 读取活动区和交换区的状态 */
	ee_flashRead(indexStartAddr, (ee_uint8*)&regionStatus, 4);
	ee_flashRead(indexSwapStartAddr, (ee_uint8*)&swapRegionStatus, 4);

	switch (regionStatus)
	{
		case REGION_ACTIVE:

			switch (swapRegionStatus)
			{
				case REGION_ERASING:
					/* 正常情况 */
					flashMemMangHandle_Init(pobj, indexStartAddr, indexSwapStartAddr, indexRegionSize, indexSize, dataStartAddr, dataSwapStartAddr, dataRegionSize);
					break;

				case REGION_COPY:
				case REGION_VERIFIED:
					/* 当活动区avtive，交换区处于copy或者varified状态说明都要进行区域交换 */
					flashMemMangHandle_Init(pobj, indexStartAddr, indexSwapStartAddr, indexRegionSize, indexSize, dataStartAddr, dataSwapStartAddr, dataRegionSize);

					/* 交换索引区域 */
					swapRegion(pobj);
					break;
			}
			break;
			
		case REGION_ERASING:

			switch(swapRegionStatus)
			{
				case REGION_ERASING:
					/* 第一次使用 */
					flashMemMangHandle_Init(pobj, indexStartAddr, indexSwapStartAddr, indexRegionSize, indexSize, dataStartAddr, dataSwapStartAddr, dataRegionSize);

					/* 第一次使用擦除所有要用的区域 */
					eraseRegion(pobj, indexStartAddr);
					eraseRegion(pobj, indexSwapStartAddr);
					eraseRegion(pobj, dataStartAddr);
					eraseRegion(pobj, dataSwapStartAddr);

					if(!verifyRegionFullyErased(pobj,indexStartAddr))
					{
						regionStatus = REGION_ACTIVE;

						ee_flashWrite(indexStartAddr, (ee_uint8*)&regionStatus, 4);
					}
					break;

				case REGION_ACTIVE:
					/* 活动在交换区 */
					flashMemMangHandle_Init(pobj, indexSwapStartAddr, indexStartAddr, indexRegionSize, indexSize, dataStartAddr, dataSwapStartAddr, dataRegionSize);
					break;
			}

		case REGION_COPY:
		case REGION_VERIFIED:
			/* 说明之前活动在交换区，在进行区域交换时被终止*/
			if(swapRegionStatus == REGION_ACTIVE)
			{
				flashMemMangHandle_Init(pobj, indexSwapStartAddr, indexStartAddr, indexRegionSize, indexSize, dataStartAddr, dataSwapStartAddr, dataRegionSize);

				swapRegion(pobj);
			}
			break;

		default:
			/* 如果都不是上面的状态，说明区域状态异常，初始化为最初的状态 */
			flashMemMangHandle_Init(pobj, indexStartAddr, indexSwapStartAddr, indexRegionSize, indexSize, dataStartAddr, dataSwapStartAddr, dataRegionSize);

			/* 擦除所有要用的区域 */
			eraseRegion(pobj, indexStartAddr);
			eraseRegion(pobj, indexSwapStartAddr);
			eraseRegion(pobj, dataStartAddr);
			eraseRegion(pobj, dataSwapStartAddr);

			if(!verifyRegionFullyErased(pobj,indexStartAddr))
			{
				regionStatus = REGION_ACTIVE;

				ee_flashWrite(indexStartAddr, (ee_uint8*)&regionStatus, 4);
			}
			break;
	}
}

/**
 * @brief: 初始化flash管理句柄
 */
static void flashMemMangHandle_Init(flash_MemMang_t* pobj,       \
                  ee_uint32 indexStartAddr,                      \
				  ee_uint32 indexSwapStartAddr,                  \
				  ee_uint16 indexRegionSize,                     \
				  ee_uint16 indexSize,                           \
				  ee_uint32 dataStartAddr,                       \
				  ee_uint32 dataSwapStartAddr,                   \
				  ee_uint16 dataRegionSize)         
{
	/* 索引区初始化 */
	pobj->indexRegionSize = indexRegionSize;
	pobj->indexAreaSize = indexSize;
	/* 空4字节存储区域状态 */
	pobj->indexStartAddr = indexStartAddr + 4;
	pobj->indexSwapStartAddr = indexSwapStartAddr + 4;
	/* 计算重写计算区的大小 */
	pobj->overwriteCountAreaSize = 1.0 * SECTORS(indexRegionSize - indexSize) / sizeof(ee_dataIndex) / 8 + 0.5;
	/* 4字节对齐 */
	pobj->overwriteCountAreaSize = (pobj->overwriteCountAreaSize + 0x03) & (~0x03);

	/* 重写区实际地址 */
	/* TODO: bugs 由于交换区，的存在，每次此单片机复位后indexStartAddr的值可能是交换区的地址，同时overwriteAddr的值也要替换成交换区的 */
	pobj->overwriteAddr = pobj->indexStartAddr + SECTORS(pobj->indexAreaSize) + pobj->overwriteCountAreaSize;

	/* 数据区初始化 */
	pobj->dataRegionSize = dataRegionSize;
	/* 数据区不用空4字节存储区域状态 */
	pobj->dataStartAddr = dataStartAddr;
	pobj->dataSwapStartAddr = dataSwapStartAddr;
}

ee_uint8 ee_writeDataToFlash(flash_MemMang_t* pobj, void* buf, ee_uint16 bufSize, variableLists dataId)
{
	ee_dataIndex currentdataIndex;
	ee_uint32 dataRegionFreeAddr = 0;
	ee_uint32 writeIndexAddr = pobj->indexStartAddr + sizeof(ee_dataIndex) * dataId;

	/* 写入的数据超过索引区，直接返回 */
	if(writeIndexAddr >= pobj->overwriteAddr)
	{
		return 1;
	}

	if(dataId != 0)
	{
		ee_dataIndex preDataIndex;

		/* 获取上一个数据索引有没有写入 */
		ee_flashRead(pobj->indexStartAddr + sizeof(ee_dataIndex) * (dataId - 1), (ee_uint8*)&preDataIndex, sizeof(preDataIndex));

		/* 上一个数据索引没有dataAddr写入直接返回 */
		if((preDataIndex.dataStatus == DATA_EMPTY) || (preDataIndex.dataStatus == DATA_INVALID))
		{
			/* 顺序存储的主要目的，就是为了快速定位数据区的空闲地址 */
			return 2;
		}
	}

	/* 获得数据区空闲区地址 */
	dataRegionFreeAddr = getFreeAddrInDataRegion(pobj);

	/* 获取当前数据索引的信息 */
	ee_flashRead(writeIndexAddr, (ee_uint8*)&currentdataIndex, sizeof(currentdataIndex));
	
	/* 当数据状态是valid或者invalid或者haldvalid都要向重写区重新写入新的数据索引 */
	/* 如果状态为invalid或halfvaild说明上次写入时，单片机断电或者复位了 */
	if(currentdataIndex.dataStatus != DATA_EMPTY)
	{
		/* NOTE: 若数据状态是invalid或halfvalid时，因为无法保证下一次在在索引区相同地址写入时，
		 * 索引数据的大小和上次写入失败时是一样的，因此舍弃索引区的数据索引，在重写区重新写入 */
		ee_uint32 lastIndexAddr, overwriteAreaFreeAddr, overwriteAreaBiasAddr; // 支持扩展寻址空间到4GB

		/* 首先找到最后一个没被重写的数据索引地址 */
		lastIndexAddr = getLastIndexAddrThatNotBeenOverwritten(pobj, dataId);

		/* 再找到重写区空闲位置的地址 */
		overwriteAreaFreeAddr = getFreeAddrInOverwriteArea(pobj);

		/* 写入索引的重写地址是偏移地址 */
		overwriteAreaBiasAddr = overwriteAreaFreeAddr - pobj->overwriteAddr;

		/* 若重写区或数据区溢出 */
		if((overwriteAreaFreeAddr + sizeof(ee_dataIndex)) > (pobj->indexStartAddr + SECTORS(pobj->indexRegionSize)) ||\
		   (dataRegionFreeAddr + bufSize) > SECTORS(pobj->dataRegionSize))
		{
			/* 交换活动空间 */
			swapRegion(pobj);

			/* 重新获得数据区空闲地址 */
			dataRegionFreeAddr = getFreeAddrInDataRegion(pobj);

			/* 刚交换完空间， 最后一个没被重写的数据索引地址即索引区的数据索引 */
			lastIndexAddr = writeIndexAddr;

			/* 重置重写区写入地址为起始地址，因为刚交换好区域，重写区空闲地址就是重写区起始地址 */
			overwriteAreaFreeAddr  = pobj->overwriteAddr;

			/* 重写区第一个数据地址偏移为0 */
			overwriteAreaBiasAddr = 0;
		}

		/* 准备写入前，首先先把重写计数+1，以防写入时单片机断电或复位导致数据没有写入成功 */
		countAreaPlusOne(pobj);

		/* 将索引写入重写区，数据写入数据区 */
		writeIndexAndData(pobj, dataRegionFreeAddr, overwriteAreaFreeAddr, buf, bufSize);

		/* 最后将上一个索引的重写地址设置为当前刚刚写入的索引地址(一定是最后设置) */
		/* 如果程序在这里中断(没有进函数)，重写区将会出现一个valid的数据索引但是没有人指向它(没有索引知道它的存在)，因此也会被程序当成一个无效索引而跳过 */
		ee_flashWrite(lastIndexAddr + sizeof(ee_dataIndex) - sizeof(currentdataIndex.dataOverwriteAddr), (ee_uint8*)&overwriteAreaBiasAddr, sizeof(currentdataIndex.dataOverwriteAddr));
	}
	else /* 状态为empty，说明是第一次写入 */
	{
		/* 将索引写入索引区 */
		writeIndexAndData(pobj, dataRegionFreeAddr, writeIndexAddr, buf, bufSize);
	}

	return 0;
}

ee_uint8 ee_readDataFromFlash(flash_MemMang_t* pobj, void* buf, variableLists dataId)
{
	ee_dataIndex readIndex;
	ee_uint32 readIndexAddr = pobj->indexStartAddr + sizeof(ee_dataIndex) * dataId;

	/* 读取的数据超过索引区，直接返回 */
	if(readIndexAddr >= pobj->overwriteAddr)
	{
		return 1;
	}

	/* 获取当前数据索引的信息 */
	ee_flashRead(readIndexAddr, (ee_uint8*)&readIndex, sizeof(readIndex));

	if(readIndex.dataStatus != DATA_EMPTY)
	{
		/* 当前数据被重写了。因为重写索引地址是在重写流程的最后才被赋值的，
		 * 所以程序保证只要数据被重写那么被重写的索引一定是可用的 */
		if(readIndex.dataOverwriteAddr != (ee_uint16)0xFFFF)
		{
			/* 获取最后一个没被重写的索引 */
			ee_uint32 lastIndexAddr = getLastIndexAddrThatNotBeenOverwritten(pobj, dataId);

			/* 获取最后一个重写索引的信息 */
			ee_flashRead(lastIndexAddr, (ee_uint8*)&readIndex, sizeof(readIndex));

			/* 去数据区读数据 */		
			ee_flashRead(readIndex.dataAddr + pobj->dataStartAddr,(ee_uint8*)buf, readIndex.dataSize);
		}
		else if(readIndex.dataStatus == DATA_VALID)
		{
			/* 当前数据重写没有被重写，同时数据有效，直接去数据区读数据 */		
			ee_flashRead(readIndex.dataAddr + pobj->dataStartAddr,(ee_uint8*)buf, readIndex.dataSize);
		}
		else
		{
			/* 来到这里说明当前索引的数据不是有效的，同时还没有被重写过(或者重写失败了) */
			return 3;
		}
	}
	else /* 数据为空，直接返回 */
	{
		return 2;
	} 

	return 0;
}

static void writeIndexAndData(flash_MemMang_t* pobj, ee_uint32 writeDataAddr, ee_uint32 writeIndexAddr, void* buf, ee_uint16 bufSize)
{
	ee_dataIndex dataIndex;
	/* 写入前，先将当前数据索引设置为invalid状态 */
	dataIndex.dataStatus = DATA_INVALID;

	/* 写入当前状态 */
	ee_flashWrite(writeIndexAddr, (ee_uint8*)&dataIndex, sizeof(dataIndex.dataStatus));

	/* 现在将剩余结构成员写入 */
	dataIndex.dataSize = bufSize;
	dataIndex.dataAddr = writeDataAddr;
	dataIndex.dataOverwriteAddr = 0xFFFF;
	ee_flashWrite(writeIndexAddr + sizeof(dataIndex.dataStatus), (ee_uint8*)&dataIndex.dataSize, sizeof(dataIndex) - sizeof(dataIndex.dataStatus));

	/* 写入数据前，将当前数据索引设置为halfvalid状态 */
	dataIndex.dataStatus = DATA_HALFVALID;
	ee_flashWrite(writeIndexAddr, (ee_uint8*)&dataIndex, sizeof(dataIndex.dataStatus));

	/* 将真正的数据写入数据区 */
	ee_flashWrite(pobj->dataStartAddr + dataIndex.dataAddr, (ee_uint8*)buf, dataIndex.dataSize);

	/* 写入后，将当前数据索引设置为valid状态 */
	dataIndex.dataStatus = DATA_VALID;
	ee_flashWrite(writeIndexAddr, (ee_uint8*)&dataIndex, sizeof(dataIndex.dataStatus));
}
/**
 * @brief: 获取最后一个没有被重写的索引地址(直接访问地址，不是偏移地址)
 */
static ee_uint32 getLastIndexAddrThatNotBeenOverwritten(flash_MemMang_t* pobj, variableLists dataId)
{
	ee_dataIndex dataIndex;
	ee_uint32 currentIndexAddr;
	ee_uint32 currentOverwriteAddr;
	ee_uint32 nextIndexAddr = pobj->indexStartAddr + sizeof(ee_dataIndex) * dataId;

	do
	{
		/* 保存当前索引的地址 */
		currentIndexAddr = nextIndexAddr;

		/* 读下一个数据索引的地址 */
		ee_flashRead(currentIndexAddr, (ee_uint8*)&dataIndex, sizeof(dataIndex));

		/* 读取当前索引的重写地址 */
		currentOverwriteAddr =  dataIndex.dataOverwriteAddr;

		/* 获取下一个索引的地址 */
		nextIndexAddr = pobj->overwriteAddr + currentOverwriteAddr;
	}while(currentOverwriteAddr != (ee_uint16)0xFFFF);

	return currentIndexAddr;
}

/**
 * @brief: 获取数据区空闲的地址(返回相对于dataStartAddr的偏移地址)
 */
static ee_uint32 getFreeAddrInDataRegion(flash_MemMang_t* pobj)
{
	ee_uint32 freeAddr = 0;
	ee_uint32 lastIndexAddr = 0;
	ee_dataIndex lastDataIndex;

	/* 确保变量表中有变量 */
	if(DATA_NUM != 0)
	{
		lastIndexAddr = pobj->indexStartAddr + sizeof(ee_dataIndex) * (DATA_NUM - 1);
	}

	/* 获取索引区指向数据区最大的地址 */
	while (lastIndexAddr >= pobj->indexStartAddr)
	{
		/* 获取索引区最后一个索引的数据 */
		ee_flashRead(lastIndexAddr, (ee_uint8*)&lastDataIndex, sizeof(lastDataIndex));

		/* 如果最后一个数据索引的状态有效，则获得最后一个数据索引数据的地址，否则代表最后一个数据地址无用， 继续从后往前寻找有效的数据索引 */
		if((lastDataIndex.dataStatus == DATA_VALID) || (lastDataIndex.dataStatus == DATA_HALFVALID))
		{
			/* halfvalid代表我上次在数据区写着写着，你把我单片机给扬喽，因此这块的数据我也不要嘞 */
			freeAddr = lastDataIndex.dataAddr + lastDataIndex.dataSize;

			break;
		}

		/* 避免(indexStartAddr为0时)减法导致的数据类型溢出 */
		if(lastIndexAddr == pobj->indexStartAddr) break;

		/* 地址递减 */
		lastIndexAddr -= sizeof(lastDataIndex);
	}

	/* 获取重写区空闲的地址 */
	lastIndexAddr = getFreeAddrInOverwriteArea(pobj);

	/* 如果重写区有重写的数据索引，获取指向数据区最大的地址 */
	if(lastIndexAddr != pobj->overwriteAddr)
	{
		/* 定位重写区最后一个数据索引的地址 */
		lastIndexAddr = lastIndexAddr - sizeof(lastDataIndex);

		while(lastIndexAddr >= pobj->overwriteAddr)
		{
			/* 获取重写区最后一个索引的数据 */
			ee_flashRead(lastIndexAddr, (ee_uint8*)&lastDataIndex, sizeof(lastDataIndex));

			/* 如果最后一个数据索引的状态处于有效或者半有效状态 */
			if((lastDataIndex.dataStatus == DATA_VALID) || (lastDataIndex.dataStatus == DATA_HALFVALID))
			{
				/* 重写区最后一个有效的数据索引指向数据区的地址，大于索引区最大指向数据区的地址 */
				if(freeAddr < (lastDataIndex.dataAddr + lastDataIndex.dataSize))
				{
					/* 说明当前索引指向的地址后面是空闲的数据空间 */
					/* 半有效状态，说明在向数据区写入数据时单片机终止运行，这里的做法就是直接抛弃数据区的这一片存储空间 */
					freeAddr = lastDataIndex.dataAddr + lastDataIndex.dataSize;
				}

				break;
			}

			lastIndexAddr -= sizeof(lastDataIndex);
		}
	}
	
	return freeAddr;
}

/**
 * @brief:  重写计数区计数加一
 */
static void countAreaPlusOne(flash_MemMang_t* pobj)
{
	ee_uint16 i;
	ee_uint32 addressValue = 0;
	ee_uint32 countAreaAddr = pobj->overwriteAddr - pobj->overwriteCountAreaSize;
	
	for(i = 0; i < pobj->overwriteCountAreaSize; i += 4)
	{
		ee_flashRead(countAreaAddr + i, (ee_uint8*)&addressValue, sizeof(addressValue));

		/* 如果读到的值全是0，代表当前四字节已经被计数满了 */
		if(addressValue == (ee_uint32)0x00000000)
		{
			/* 读下一个地址 */
			continue;
		}
		
		/* 来到这里说明四字节里还可以进行计数 */
		addressValue <<= 1;
		
		/* 将计数写入计数区 */
		ee_flashWrite(countAreaAddr + i, (ee_uint8*)&addressValue, sizeof(addressValue));

		/* 计数结束 退出*/
		break;
	}
}



/**
 * @brief:  获取重写区空闲的地址(直接访问地址，不是偏移地址)
 */
static ee_uint32 getFreeAddrInOverwriteArea(flash_MemMang_t* pobj)
{
	ee_uint16 i;
	ee_uint16 overwriteCount = 0;
	ee_uint32 addressValue = 0xFFFFFFFF;
	ee_uint32 countAreaAddr = pobj->overwriteAddr - pobj->overwriteCountAreaSize;

	/* 获取重写区一共重写了多少个数据 */
	for(i = 0; i < pobj->overwriteCountAreaSize; i += 4)
	{
		ee_flashRead(countAreaAddr + i, (ee_uint8*)&addressValue, sizeof(addressValue));

		/* 获取一个addressValue中0的个数 */
		if(addressValue != (ee_uint32)0xFFFFFFFF)
		{
			ee_int32 num = addressValue;

			while(num + 1)
			{
				overwriteCount++;
				num |= (num + 1);
			}
		}
		else
		{
			break;
		}
	}

	return (pobj->overwriteAddr + sizeof(ee_dataIndex) * overwriteCount);
}

/**
 * @brief: 检查区域是否完全被擦除
 * @retval: 0: region erased, 1: region not erased
 */
static ee_uint8 verifyRegionFullyErased(flash_MemMang_t *pobj, ee_uint32 regionAddr)
{
	ee_uint8 ret = 0;
	ee_uint32 addressValue = 0;
	ee_uint32 regionEndAddr = 0;

	/* 获取当前区的结束地址 */
	if((pobj->indexStartAddr == regionAddr) || (pobj->indexSwapStartAddr == regionAddr))
	{
		regionEndAddr = regionAddr + SECTORS(pobj->indexRegionSize);
	}
	else
	{
		regionEndAddr = regionAddr + SECTORS(pobj->dataRegionSize);
	}

	/* Check each active page address starting from end */
	while (regionAddr < regionEndAddr)
	{
		/* Get the current location content to be compared with virtual address */
		ee_flashRead(regionAddr, (ee_uint8*)&addressValue, sizeof(addressValue));

		/* Compare the read address with the virtual address */
		if (addressValue != (ee_uint32)0xFFFFFFFF)
		{
			/* In case variable value is read, reset ReadStatus flag */
			ret = 1;

			break;
		}

		/* Next address location */
		regionAddr = regionAddr + sizeof(addressValue);
	}

	/* Return ReadStatus value: (0: region erased, 1: region not erased) */
	return ret;
}

/**
 * @brief: 擦除一个区域
 */
static void eraseRegion(flash_MemMang_t *pobj, ee_uint32 regionAddr)
{
	ee_uint32 regionEndAddr = 0;

	if((pobj->indexStartAddr == regionAddr) || (pobj->indexSwapStartAddr == regionAddr))
	{
		regionEndAddr = regionAddr + SECTORS(pobj->indexRegionSize);
	}
	else
	{
		regionEndAddr = regionAddr + SECTORS(pobj->dataRegionSize);
	}

	while (regionAddr < regionEndAddr)
	{
		ee_flashEraseASector(regionAddr);

		regionAddr += SECTOR_SIZE;
	}
}


static void transferDataAndIndex(flash_MemMang_t* pobj, ee_dataIndex* pindex, ee_uint32* newDataAddr, ee_uint32 newIndexAddr)
{
	ee_uint32 i;
	/* 修改数据在交换区新的地址 */
	pindex->dataAddr = *newDataAddr;

	/*将索引写入交换区 */
	ee_flashWrite(newIndexAddr, (ee_uint8*)pindex, sizeof(ee_dataIndex));

	/* 将数据从活动区中读出，并写入交换数据区 */
	for(i = 0; i < pindex->dataSize; i++)
	{
		ee_uint8 data = 0;

		/* 从满数据区中读出 */
		ee_flashRead(pindex->dataAddr + pobj->dataStartAddr, &data, 1);

		/* 写入到新交换数据区 */
		ee_flashWrite(*newDataAddr, &data, 1);

		/* 地址递增 */
		*newDataAddr += 1;
	}
}

static void swapData(flash_MemMang_t* pobj)
{
	ee_uint32 i;
	ee_uint32 tmp;
	ee_uint32 regionStatus = 0;
	ee_uint32 swapRegionAddr = 0;

	/* 在拷贝数据前将交换区状态设置为copy */
	regionStatus = REGION_COPY;
	ee_flashWrite(pobj->indexSwapStartAddr - 4, (ee_uint8*)&regionStatus, 4);
	
	/* 开始将所有数据索引拷贝到交换区域 */
	for(i = 0; i < DATA_NUM; i++)
	{
		ee_dataIndex readIndex;
		ee_uint32 readIndexAddr = pobj->indexStartAddr + sizeof(ee_dataIndex) * i;
		ee_uint32 writeIndexAddr = pobj->indexSwapStartAddr + sizeof(ee_dataIndex) * i;

		/* 获取当前数据索引的信息 */
		ee_flashRead(readIndexAddr, (ee_uint8*)&readIndex, sizeof(readIndex));

		if(readIndex.dataStatus != DATA_EMPTY)
		{
			if(readIndex.dataOverwriteAddr != (ee_uint16)0xFFFF)
			{
				/* 获取最后一个没被重写的索引 */
				ee_uint32 lastIndexAddr = getLastIndexAddrThatNotBeenOverwritten(pobj, (variableLists)i);

				/* 获取最后一个重写索引的信息 */
				ee_flashRead(lastIndexAddr, (ee_uint8*)&readIndex, sizeof(readIndex));

				/* 传输数据和索引到交换区 */
				transferDataAndIndex(pobj, &readIndex, &swapRegionAddr, writeIndexAddr);
			}
			else if(readIndex.dataStatus == DATA_VALID)
			{
				/* 传输数据和索引到交换区 */
				transferDataAndIndex(pobj, &readIndex, &swapRegionAddr, writeIndexAddr);
			}
		}
	}

	/* 交换索引活动区 */
	tmp = pobj->indexStartAddr;
	pobj->indexStartAddr = pobj->indexSwapStartAddr;
	pobj->indexSwapStartAddr = tmp;
	/* 更新重写区在新的活动区的地址 */
	pobj->overwriteAddr = pobj->indexStartAddr + SECTORS(pobj->indexAreaSize) + pobj->overwriteCountAreaSize;
	
	/* 交换数据活动区 */
	tmp = pobj->dataStartAddr;
	pobj->dataStartAddr = pobj->dataSwapStartAddr;
	pobj->dataSwapStartAddr = tmp;

	/* 下面两条写入状态语句，将活动区的copy变为active，将交换区的active变为erasing
	 * 由于持续时间很短，因此我们认为不会同时出现两个active的情况 */
	/* 在拷贝数据后将状态设置为active */
	regionStatus = REGION_ACTIVE;
	ee_flashWrite(pobj->indexStartAddr - 4, (ee_uint8*)&regionStatus, 4);

	/* 擦除前将状态设置为erasing */
	regionStatus = REGION_ERASING;
	ee_flashWrite(pobj->indexSwapStartAddr - 4, (ee_uint8*)&regionStatus, 4);

	/* 擦除交换区 */
	eraseRegion(pobj, pobj->indexSwapStartAddr - 4);
	eraseRegion(pobj, pobj->dataSwapStartAddr);
}

/**
 * @brief: 当数据区溢出或者数据索引区溢出时，都需要进行调换活动区
 * 索引区溢出或数据区溢出：调换数据区和数据索引区的活动区(最初想法如果只有索引区溢出只交换索引区
 * 这个方案可行但是会导致拥有4个区域状态，导致初始化代码复杂)
 * TODO :在交换区域后，将之前的 indexStartAddr 赋值为之前的 indexSwapStartAddr，而之前的indexSwapStartAddr 赋值为之前的 indexStartAddr，每交换一次操作执行一次此操作，这样做方便其他函数编写程序(程序专注于indexStartAddr，不必在意indexSwapStartAddr)
 */
static void swapRegion(flash_MemMang_t* pobj)
{
	ee_uint32 regionStatus = 0;

	/* 读交换区的状态 */
	ee_flashRead(pobj->indexSwapStartAddr - 4, (ee_uint8*)&regionStatus, 4);

	switch(regionStatus)
	{
		/* 发现当前active交换区状态为copy，说明在拷贝数据时单片机终止运行 */
		/* 发现当前active交换区状态为active，说明数据拷贝完，但是还没开始擦除的情况 */
		/* 发现当前active交换区状态为erasing，说明区域可能被擦除了也可能在擦除时被终止了，因此都需要先验证是否完全被擦除 */
		case REGION_COPY: 
		case REGION_ACTIVE:
		case REGION_ERASING:
			if(verifyRegionFullyErased(pobj, pobj->indexSwapStartAddr) || \
			   verifyRegionFullyErased(pobj, pobj->dataSwapStartAddr))
			{
				/* 如果验证块没有被完全擦除则进行块的擦除 */
				/* 如果在擦除时，单片机复位，区的状态还是处于erased */
				eraseRegion(pobj, pobj->indexSwapStartAddr - 4);
				eraseRegion(pobj, pobj->dataSwapStartAddr);
			}

			/* 将区的状态设置为verified */
			regionStatus = REGION_VERIFIED;
			ee_flashWrite(pobj->indexSwapStartAddr - 4, (ee_uint8*)&regionStatus, 4);

			/* 验证状态之间进行交换 */
		case REGION_VERIFIED:
			/* 交换索引区 */
			swapData(pobj);
			break;
	}
}

