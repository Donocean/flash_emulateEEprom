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
#define BLOCK_ERASED       ((ee_uint32)0xFFFFFFFF)
#define BLOCK_VERIFIED     ((ee_uint32)0x00FFFFFF)
#define BLOCK_COPY         ((ee_uint32)0x0000FFFF)
#define BLOCK_ACTIVE       ((ee_uint32)0x000000FF)

/* 数据状态 */
#define DATA_EMPTY         ((ee_uint16)0xFFFF)
#define DATA_INVALID       ((ee_uint16)0x00FF)
#define DATA_VALID         ((ee_uint16)0x0000)

/* 数据索引结构 */
typedef struct 
{
	/* 当前数据状态 */
	ee_uint16 dataStatus;
	/* 当前数据大小 */
	ee_uint16 dataSize;
	/* 当前数据在数据区的地址 */
	ee_uint16 dataAddr;
	/* 当前数据被重写的地址，默认为0xFFFF */
	ee_uint16 dataOverwriteAddr;
}ee_dataIndex;

static ee_uint32 ee_getFreeAddrInDataRegion(flash_MemMang_t* pobj);
static ee_uint32 ee_getFreeAddrInOverwriteArea(flash_MemMang_t* pobj);
static void ee_countAreaPlusOne(flash_MemMang_t* pobj);
static void ee_writeIndexAndData(flash_MemMang_t* pobj, ee_uint32 writeIndexAddr, void* buf, ee_uint16 bufSize);
static ee_uint32 ee_getLastIndexNotBeenOverwrittenAddr(flash_MemMang_t* pobj, variableLists dataId);
static ee_uint8 VerifyRegionFullyErased(flash_MemMang_t *pobj, ee_uint32 regionAddr);


// flash管理指针
// 索引区起始地址
// 索引交换区起始地址
// 索引区总大小
// 索引区大小(用于定位重写区地址)
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
	/* 索引区初始化 */
	pobj->indexRegionSize = indexRegionSize;
	/* 空4字节存储区域状态 */
	pobj->indexStartAddr = indexStartAddr + 4;
	pobj->indexSwapStartAddr = indexSwapStartAddr + 4;
	/* 计算重写计算区的大小 */
	pobj->overwriteCountAreaSize = 1.0 * SECTORS(indexRegionSize - indexSize) / sizeof(ee_dataIndex) / 8 + 0.5;

	/* 4字节对齐 */
	pobj->overwriteCountAreaSize = (pobj->overwriteCountAreaSize + 0x03) & (~0x03);

	/* 重写区实际地址 */
	pobj->overwriteAddr = indexStartAddr + SECTORS(indexSize) + pobj->overwriteCountAreaSize;

	/* 数据区初始化 */
	pobj->dataRegionSize = dataRegionSize;
	/* 空4字节存储区域状态 */
	pobj->dataStartAddr = dataStartAddr + 4;
	pobj->dataSwapStartAddr = dataSwapStartAddr + 4;

}


ee_uint8 ee_writeDataToFlash(flash_MemMang_t* pobj, void* buf, ee_uint16 bufSize, variableLists dataId)
{
	ee_dataIndex currentdataIndex;
	ee_uint32 writeIndexAddr = pobj->indexStartAddr + sizeof(ee_dataIndex) * dataId;

	/* 写入的数据超过数据索引区，直接返回 */
	if(writeIndexAddr >= pobj->overwriteAddr)
	{
		return 1;
	}

	if(dataId != 0)
	{
		ee_dataIndex preDataIndex = {0};

		/* 获取上一个数据索引有没有写入 */
		ee_flashRead(pobj->indexStartAddr + sizeof(ee_dataIndex) * (dataId - 1), (ee_uint8*)&preDataIndex, sizeof(preDataIndex));

		/* 上一个数据索引没有写入直接返回 */
		if(preDataIndex.dataStatus == DATA_EMPTY)
		{
			return 2;
		}
	}

	/* 获取当前数据索引的信息 */
	ee_flashRead(writeIndexAddr, (ee_uint8*)&currentdataIndex, sizeof(currentdataIndex));
	
	/* 当前数据非有效 */
	if(currentdataIndex.dataStatus != DATA_VALID)
	{
		/* 状态为empty，说明是第一次写入 */
		if(currentdataIndex.dataStatus == DATA_EMPTY)
		{
			/* 将索引和数据写入 */
			ee_writeIndexAndData(pobj, writeIndexAddr, buf, bufSize);
		}
		else /* 状态为invalid说明上次写入时，单片机断电或者复位了 */
		{

		}

	}
	else /* 当前数据是重新写入 */
	{
		ee_uint32 lastIndexAddr, overwriteFreeAddr;

		/* 首先找到最后一个没被重写的数据索引地址 */
		lastIndexAddr = ee_getLastIndexNotBeenOverwrittenAddr(pobj, dataId);
		/* 再找到重写区空闲位置的地址 */
		overwriteFreeAddr = ee_getFreeAddrInOverwriteArea(pobj);

		/* 准备写入前，首先先把重写计数+1，避免写入时，单片机断电或复位导致数据没有写入成功 */
		ee_countAreaPlusOne(pobj);

	}
	

	return 0;
}

ee_uint8 readDataFromFlash(flash_MemMang_t* pobj, void* dataAddr, variableLists dataId)
{

	return 0;
}

static void ee_writeIndexAndData(flash_MemMang_t* pobj, ee_uint32 writeIndexAddr, void* buf, ee_uint16 bufSize)
{
	ee_dataIndex dataIndex;
	/* 写入前，先将当前数据索引设置为invalid状态 */
	dataIndex.dataStatus = DATA_INVALID;

	/* 写入当前状态 */
	ee_flashWrite(writeIndexAddr, (ee_uint8*)&dataIndex, sizeof(dataIndex.dataStatus));

	/* 现在将剩余数据索引写入 */
	dataIndex.dataSize = bufSize;
	dataIndex.dataAddr = ee_getFreeAddrInDataRegion(pobj);
	dataIndex.dataOverwriteAddr = 0xFFFF;
	ee_flashWrite(writeIndexAddr + sizeof(dataIndex.dataStatus), (ee_uint8*)&dataIndex.dataSize, sizeof(dataIndex) - sizeof(dataIndex.dataStatus));

	/* 将真正的数据写入数据区 */
	ee_flashWrite(dataIndex.dataAddr, (ee_uint8*)buf, dataIndex.dataSize);

	/* 写入后，将当前数据索引设置为valid状态 */
	dataIndex.dataStatus = DATA_VALID;

	/* 写入当前状态 */
	ee_flashWrite(writeIndexAddr, (ee_uint8*)&dataIndex, sizeof(dataIndex.dataStatus));
}
/**
 * @brief: 获取最后一个没有被重写的索引地址
 */
static ee_uint32 ee_getLastIndexNotBeenOverwrittenAddr(flash_MemMang_t* pobj, variableLists dataId)
{
	ee_dataIndex dataIndex;
	ee_uint32 currentIndexAddr;
	ee_uint32 nextIndexAddr = pobj->indexStartAddr + sizeof(ee_dataIndex) * dataId;

	do
	{
		/* 保存当前索引的地址 */
		currentIndexAddr = nextIndexAddr;

		/* 读下一个数据索引的地址 */
		ee_flashRead(currentIndexAddr, (ee_uint8*)&dataIndex, sizeof(dataIndex));

		/* 获取下一个索引的地址 */
		nextIndexAddr = dataIndex.dataOverwriteAddr;
	}while(nextIndexAddr != (ee_uint16)0xFFFF);

	return currentIndexAddr;
}

/**
 * @brief: 获取数据区空闲的地址
 */
static ee_uint32 ee_getFreeAddrInDataRegion(flash_MemMang_t* pobj)
{
	ee_dataIndex lastDataIndex;
	ee_uint32 lastIndexAddr = 0;
	ee_uint32 freeAddr = pobj->dataStartAddr;

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
		if(lastDataIndex.dataStatus == DATA_VALID)
		{
			freeAddr = lastDataIndex.dataAddr + lastDataIndex.dataSize;

			break;
		}

		/* 避免(indexStartAddr为0时)减法导致的数据溢出 */
		if(lastIndexAddr == pobj->indexStartAddr) break;

		lastIndexAddr -= sizeof(lastDataIndex);
	}

	/* 获取重写区空闲的地址 */
	lastIndexAddr = ee_getFreeAddrInOverwriteArea(pobj);

	/* 如果重写区有重写的数据索引，获取指向数据区最大的地址 */
	if(lastIndexAddr != pobj->overwriteAddr)
	{
		/* 定位重写区最后一个数据索引的地址 */
		lastIndexAddr = ee_getFreeAddrInOverwriteArea(pobj) - sizeof(lastDataIndex);

		while(lastIndexAddr >= pobj->overwriteAddr)
		{
			/* 获取重写区最后一个索引的数据 */
			ee_flashRead(lastIndexAddr, (ee_uint8*)&lastDataIndex, sizeof(lastDataIndex));

			/* 如果最后一个数据索引的状态有效，则获得最后一个数据索引数据的地址，否则代表最后一个数据地址无用， 继续从后往前寻找有效的数据索引 */
			if(lastDataIndex.dataStatus == DATA_VALID)
			{
				/* 若重写区最后一个有效的数据索引指向数据区的地址大于索引区最大指向数据区的地址 */
				if(freeAddr < (lastDataIndex.dataAddr + lastDataIndex.dataSize))
				{
					/* 说明当前索引指向的地址后面是空闲的数据空间 */
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
static void ee_countAreaPlusOne(flash_MemMang_t* pobj)
{
	ee_uint16 i;
	ee_uint32 addressValue = 0;
	ee_uint32 countAreaAddr = pobj->overwriteAddr - pobj->overwriteCountAreaSize;
	
	for(i = 0; i < pobj->overwriteCountAreaSize; i += 4)
	{
		ee_flashRead(countAreaAddr + i, (ee_uint8*)&addressValue, sizeof(addressValue));

		/* 如果读到的值全是0，代表当前四字节已经被计数满了 */
		if(addressValue == (ee_uint32)0x00)
		{
			/* 读下一个地址 */
			continue;
		}
		
		/* 来到这里说明四字节里还可以进行计数 */
		addressValue <<= 1;
		
		/* 计数结束 退出*/
		break;
	}
}



/**
 * @brief:  获取重写区空闲的地址 
 */
static ee_uint32 ee_getFreeAddrInOverwriteArea(flash_MemMang_t* pobj)
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
		if (addressValue != BLOCK_ERASED)
		{
			/* In case variable value is read, reset ReadStatus flag */
			ret = 1;

			break;
		}

		/* Next address location */
		regionAddr = regionAddr + sizeof(addressValue);
	}

	/* Return ReadStatus value: (0: Sector erased, 1: Page not erased) */
	return ret;
}

/**
 * @brief: 擦除一个区域
 */
static void ee_eraseRegion(flash_MemMang_t *pobj, ee_uint32 regionAddr)
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

/**
 * @brief: 当数据区溢出或者数据索引区溢出时，都需要进行调换活动区
 * 数据区溢出：调换数据区和数据索引区的活动区
 * 索引区溢出：调换数据索引区的活动区
 * TODO :在交换区域后，将之前的 indexStartAddr 赋值为之前的 indexSwapStartAddr，而之前的indexSwapStartAddr 赋值为之前的 indexStartAddr，每交换一次操作执行一次此操作，这样做方便其他函数编写程序(程序专注于indexStartAddr，不必在意indexSwapStartAddr)
 */
static void ee_regionSwap(flash_MemMang_t* pobj, ee_uint32 regionAddr)
{

	if((regionAddr == pobj->dataStartAddr) || (regionAddr == pobj->dataSwapStartAddr))
	{

	}
	else
	{
		/* 数据区溢出：调换数据区和数据索引区的活动区 */

	}

}
