/**
 * @author Donocean (1184427366@qq.com)
 * @brief 用于spi flash 的内存管理
 * @version 0.1
 * @date 2022-02-24
 * 
 * @copyright Copyright (c) 2022, Donocean
 */

#ifndef __SPI_FLASH_MEMMANG_H_
#define __SPI_FLASH_MEMMANG_H_

#include <stdlib.h>

#define BLOCK_SECTOR_NUM 16 //一个块有16个扇区
#define SECTOR_SIZE 4096 //4KB

/* spi flash 一个block(块)有16个sector(扇区)
 * 一个扇区4KB，那么一个块就为64KB 
 * spi flash写入只能进行page(页)写入，页写入时可以随意指定写几个字节，但最多只能指定写入256字节
 * flash 写入只能将1变成0，所以在写入重复的存储区域时，
 * 需要先将重复的区域擦除，flash最小的擦除单元为sector(扇区) */
typedef enum
{
    Sector0 = 0 * SECTOR_SIZE, Sector1 = 1 * SECTOR_SIZE, 
    Sector2 = 2 * SECTOR_SIZE, Sector3 = 3 * SECTOR_SIZE,
    Sector4 = 4 * SECTOR_SIZE, Sector5 = 5 * SECTOR_SIZE, 
    Sector6 = 6 * SECTOR_SIZE, Sector7 = 7 * SECTOR_SIZE,
    Sector8 = 8 * SECTOR_SIZE, Sector9 = 9 * SECTOR_SIZE, 
    Sector10 = 10 * SECTOR_SIZE, Sector11 = 11 * SECTOR_SIZE,
    Sector12 = 12 * SECTOR_SIZE, Sector13 = 13 * SECTOR_SIZE, 
    Sector14 = 14 * SECTOR_SIZE, Sector15 = 15 * SECTOR_SIZE,
}Sector_t; /* 枚举一个block的所有扇区 */

/* 块的首地址定义，此枚举只列举了32个block(块)，即共2M，如需要更多需自己定义 */
typedef enum
{
    Block0 = 0, Block1 = SECTOR_SIZE * 16,
    Block2 = Block1 * 2,Block3 = Block1 * 3,
    Block4 = Block1 * 4,Block5 = Block1 * 5,
    Block6 = Block1 * 6,Block7 = Block1 * 7,
    Block8 = Block1 * 8,Block9 = Block1 * 9,
    Block10 = Block1 * 10,Block11 = Block1 * 11,
    Block12 = Block1 * 12,Block13 = Block1 * 13,
    Block14 = Block1 * 14,Block15 = Block1 * 15,
    Block16 = Block1 * 16,Block17 = Block1 * 17,
    Block18 = Block1 * 18,Block19 = Block1 * 19,
    Block20 = Block1 * 20,Block21 = Block1 * 21,
    Block22 = Block1 * 22,Block23 = Block1 * 23, 
    Block24 = Block1 * 24,Block25 = Block1 * 25, 
    Block26 = Block1 * 26,Block27 = Block1 * 27, 
    Block28 = Block1 * 28,Block29 = Block1 * 29, 
    Block30 = Block1 * 30,Block31 = Block1 * 31, 
}Block_t; 

/* 由于flash一次只能以一个扇区的大小擦除数据，为了防止每个扇区分配的地址被擦除，
 * 因此我用每个block的 sector0 存放当前block所有扇区的配置文件 */
/* 注意：由于sector0保存配置文件，所以sector0一般不会擦除，不建议向Sector0写入数据！！！ */
extern BCB_t xBlock0;



void Block_MemMang_Init(BCB_t *xblock, Block_t xblockAddr);
size_t Malloc_BlockOfSector(BCB_t *xBlock, Sector_t SectorNum, size_t xWantedSize);

#endif /* __SPI_FLASH_MEMMANG_H_ */