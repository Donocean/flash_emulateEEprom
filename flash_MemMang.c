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

void ee_flashFormat(flash_MemMang_t* pobj,    \
	             	ee_uint32 IndexStartAddr, \
	             	ee_uint32 IndexSwapStartAddr, \
                    ee_uint16 IndexRegionSize,\
                    ee_uint32 dataStartAddr,  \
                    ee_uint32 dataSwapStartAddr,  \
					ee_uint16 dataRegionSize)
{
	
	
}

ee_uint8 writeDataToFlash(flash_MemMang_t* pobj, void* dataAddr, ee_uint16 dataSize, variableLists dataId)
{

	return 0;
}

ee_uint8 readDataFromFlash(flash_MemMang_t* pobj, void* dataAddr, variableLists dataId)
{

	return 0;
}

static void VerifyRegion(ee_uint32 regionAddr)
{
	ee_uint32 regionStatus = 0;

	flashRead(regionAddr, (ee_uint8*)&regionStatus, sizeof(regionStatus));

	switch (regionStatus)
	{
		case BLOCK_VERIFIED:
			
			break;
	
		case BLOCK_COPY:
			
			break;

		case BLOCK_ACTIVE:
			
			break;
	}

}
static ee_uint8 VerifyRegionFullyErased(flash_MemMang_t *pobj, ee_uint32 regionAddr)
{
  ee_uint8 ret = 0;
  ee_uint32 AddressValue = 0;
  ee_uint32 regionEndAddr;
  
  if(pobj->IndexStartAddr == regionAddr)
  {
	regionEndAddr = regionAddr + pobj->IndexRegionSize * SECTOR_SIZE;
  }
  else
  {
	  regionEndAddr = regionAddr + pobj->dataRegionSize * SECTOR_SIZE;
  }
    
  /* Check each active page address starting from end */
  while (regionAddr < regionEndAddr)
  {
    /* Get the current location content to be compared with virtual address */
    AddressValue = (*(__IO uint16_t*)Address);

    /* Compare the read address with the virtual address */
    if (AddressValue != BLOCK_ERASED)
    {
      /* In case variable value is read, reset ReadStatus flag */
      ReadStatus = 1;

      break;
    }
    /* Next address location */
    Address = Address + 4;
  }
  
  /* Return ReadStatus value: (0: Sector erased, 1: Page not erased) */
  return ret;
}
