/**
 * @file spi_flash_MemMang.c
 * @author Donocean (1184427366@qq.com)
 * @brief 轻量化的用于spi flash 的内存管理
 * @version 0.1
 * @date 2022-02-24
 * 
 * @copyright Copyright (c) 2022, Donocean
 */

#include "spi_flash_MemMang.h"
//#include "spi_flash.h"

typedef struct BlcockControlBlock
{
    Block_t xBlockAddr; /* 块的初始地址 */
    SCB_t xSector[BLOCK_SECTOR_NUM]; /* 当前块的扇区控制块 */
}BCB_t; /* flash的块(block)控制块 */

typedef struct SectorControlBlock
{
    Sector_t xSectorAddr;        /* 记录扇区的起始地址 */
    size_t  xNextFreeByte;       /* 记录已经分配了多少字节,主要用来定位下一个空闲的内存堆位置 */

    unsigned char ucNextFreeArea;/* 记录已经分配了多少区域 */
    /* 保存当前扇区已经分配的的区域的首地址，这里我定义一个扇区最多可以分配成256个区域(根据分配不同的区域大小，不一定能分配到256个区域) */
    size_t xAllocAreaAddr[256];  /* 向flash写入数据时遍历此数组即可，当前区域n的大小为：(xAllocAreaAddr[n+1] - xAllocAreaAddr[n] ) */
}SCB_t;

/* global variables declaration */
BCB_t g_block0; //每当使用一个扇区

/* private function declaration */
static void prvSectorControlBlock_Init(SCB_t *xSectorBlock, Block_t xblockaddr, Sector_t xSectorAddr);

/**
 * @brief 初始化内存管理某个block(块)的所有sector(扇区)
 * @param xblock      某个块的BCB
 * @param xblockAddr  块的首地址 可以参考枚举类型(Block_t)
 */
void Block_MemMang_Init(BCB_t *xblock, Block_t xblockAddr)
{
    Sector_t xSe;
    unsigned char i = 0;
    
    xblock->xBlockAddr = xblockAddr;
    /* 初始化扇区0(扇区0分开初始，原因见头文件) */  
    xblock->xSector[0].xSectorAddr = xblock->xBlockAddr + Sector0;
    /* 扇区0分配的区域0保存了16个扇区的扇区控制块(SCB) */
    xblock->xSector[0].xAllocAreaAddr[0] = Sector0;  
    xblock->xSector[0].xNextFreeByte = sizeof(xblock->xSector) / sizeof(xblock->xSector[0]);
    ++ xblock->xSector[0].ucNextFreeArea;

    /* 初始化除sector0外的所有扇区控制块 */
    for(xSe = Sector1; xSe <= Sector15; xSe += SECTOR_SIZE)
    {
        prvSectorControlBlock_Init(&(xblock->xSector[i]), xblock->xBlockAddr, xSe);
        i++;
    }
}

/**
 * @brief 从一个Block(块)中的某扇区中申请区域存放数据
 * @param xBlock      某个块的BCB
 * @param SectorNum   块的某个扇区，取值类型参考枚举类型(Sector_t)
 * @param xWantedSize 想要申请多少个字节
 * @retval 0: 申请失败 1：申请成功
 */
size_t Malloc_BlockOfSector(BCB_t *xBlock, Sector_t SectorNum, size_t xWantedSize)
{
    SCB_t *xSector;
    unsigned char index = SectorNum / 4096;

    if(xWantedSize == 0)
    {
        return 0;
    }

    xSector = &(xBlock->xSector[index]);

    /* 判断是否有可用的内存空间分配出去 */
    if((xSector->xNextFreeByte + xWantedSize) < SECTOR_SIZE)
    {
        /* 获取申请的内存空间的起始地址，更新使用了多少内存 */
        xSector->xAllocAreaAddr[xSector->ucNextFreeArea] = xSector->xSectorAddr + xSector->xNextFreeByte;
        /* 记录此扇区分配多少个区域 */
        ++ xSector->ucNextFreeArea ;
        /* 记录已经分配了多少内存 */
        xSector->xNextFreeByte += xWantedSize;
    }
}

static void prvSectorControlBlock_Init(SCB_t *xSectorBlock, Block_t xblockaddr, Sector_t xSectorAddr)
{
    xSectorBlock->xSectorAddr =  xblockaddr + xSectorAddr;
    xSectorBlock->xNextFreeByte = 0; /* 没有向此扇区写入数据 */
    xSectorBlock->ucNextFreeArea = 0;/* 此扇区没有分配区域 */
}