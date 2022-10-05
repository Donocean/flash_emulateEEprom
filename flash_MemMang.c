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

/* 数据重写区索引结构 */
typedef struct
{
	/* 当前数据大小 */
	ee_uint16 dataSize;
	/* 当前数据在数据区的地址 */
	ee_uint16 dataAddr;
	/* 当前数据被重写的地址，默认为0xFFFF */
	ee_uint16 dataOverwriteAddr;
}ee_dataOverWriteIndex;

static ee_uint8 VerifyRegionFullyErased(flash_MemMang_t *pobj, ee_uint32 regionAddr);


void ee_flashInit(flash_MemMang_t* pobj,    \
                  ee_uint32 indexStartAddr, \
				  ee_uint32 indexSwapStartAddr, \
				  ee_uint16 indexRegionSize,\
				  ee_uint16 indexOverwriteSize,\
				  ee_uint32 dataStartAddr,  \
				  ee_uint32 dataSwapStartAddr,  \
				  ee_uint16 dataRegionSize)
{
	/* 索引区初始化 */
	pobj->indexRegionSize = indexRegionSize;
	pobj->indexStartAddr = indexStartAddr;
	pobj->indexSwapStartAddr = indexSwapStartAddr;
	pobj->indexOverwriteAddr = indexStartAddr + SECTORS(indexOverwriteSize);

	/* 数据区初始化 */
	pobj->dataRegionSize = dataRegionSize;
	pobj->dataStartAddr = dataStartAddr;
	pobj->dataSwapStartAddr = dataSwapStartAddr;

}

ee_uint8 writeDataToFlash(flash_MemMang_t* pobj, void* dataAddr, ee_uint16 dataSize, variableLists dataId)
{
	/* 写入的数据超过数据索引区，直接返回 */
	if((pobj->indexStartAddr + sizeof(ee_dataIndex) * dataId) >= pobj->indexOverwriteAddr)
	{
		return 1;
	}

	

	return 0;
}

ee_uint8 readDataFromFlash(flash_MemMang_t* pobj, void* dataAddr, variableLists dataId)
{

	return 0;
}

static ee_uint8 verifyRegionFullyErased(flash_MemMang_t *pobj, ee_uint32 regionAddr)
{
	ee_uint8 ret = 0;
	ee_uint32 addressValue = 0;
	ee_uint32 regionEndAddr = 0;

	if((pobj->IndexStartAddr == regionAddr) || (pobj->IndexSwapStartAddr == regionAddr))
	{
		regionEndAddr = regionAddr + SECTORS(pobj->IndexRegionSize);
	}
	else
	{
		regionEndAddr = regionAddr + SECTORS(pobj->dataRegionSize);
	}

	/* Check each active page address starting from end */
	while (regionAddr < regionEndAddr)
	{
		/* Get the current location content to be compared with virtual address */
		e_flashRead(regionAddr, (ee_uint8*)&addressValue, sizeof(addressValue));

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

static void ee_eraseRegion(flash_MemMang_t *pobj, ee_uint32 regionAddr)
{
	ee_uint32 regionEndAddr = 0;

	if((pobj->IndexStartAddr == regionAddr) || (pobj->IndexSwapStartAddr == regionAddr))
	{
		regionEndAddr = regionAddr + SECTORS(pobj->IndexRegionSize);
	}
	else
	{
		regionEndAddr = regionAddr + SECTORS(pobj->dataRegionSize);
	}

	while (regionAddr < regionEndAddr)
	{
		e_flashEraseASector(regionAddr);

		regionAddr += SECTOR_SIZE;
	}
}

/**
 * @brief: 当数据区溢出或者数据索引区溢出时，都需要进行调换活动区
 * 数据区溢出：调换数据区和数据索引区的活动区
 * 索引区溢出：调换数据索引区的活动区
 */
static void ee_regionSwapping(flash_MemMang_t* pobj, ee_uint32 regionAddr)
{

	if( (regionAddr == pobj->dataStartAddr) || (regionAddr == pobj->dataSwapStartAddr))
	{

	}
	else
	{
		/* 数据区溢出：调换数据区和数据索引区的活动区 */

	}

}
