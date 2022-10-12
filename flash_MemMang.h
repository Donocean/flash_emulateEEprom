/**
 * @author Donocean (1184427366@qq.com)
 * @brief 用于 flash 的内存管理
 * @version 0.1
 * @date 2022-02-24
 * 
 * @copyright Copyright (c) 2022, Donocean
 */

#ifndef __FLASH_MEMMANG_H_
#define __FLASH_MEMMANG_H_

/* spi flash 一个block(块)有16个sector(扇区)
 * 一个扇区4KB，那么一个块就为64KB 
 * spi flash写入只能进行page(页)写入，页写入时可以随意指定写几个字节，但最多只能指定写入256字节
 * flash 写入只能将1变成0，所以在写入重复的存储区域时，
 * 需要先将重复的区域擦除，flash最小的擦除单元为sector(扇区) */

/* 用户根据自己单片机位数修改 */
typedef unsigned char ee_uint8;
typedef unsigned short ee_uint16;
typedef unsigned int ee_uint32;
typedef char ee_int8;
typedef short ee_int16;
typedef int ee_int32;

/* 当前flash一个扇区的大小 */
#define SECTOR_SIZE 
/* 当前flash一个块有多少个扇区 */
#define BLOCk_SECTOR_NUM 

/* 返回第x扇区的地址 */
#define SECTORS(x) (ee_uint32)((x) * SECTOR_SIZE)
/* 返回第x块的地址 */
#define BLOCKS(x)  (ee_uint32)((x) * BLOCk_SECTOR_NUM * SECTOR_SIZE)

/* 函数类型 void (*) (uint32 flashAddr, uint8* dataAddr, uint16 num) */
#define ee_flashWrite 
#define ee_flashRead  

/* 函数原型 void (*) (uint32_t flashAddr) */
#define ee_flashEraseASector 

typedef struct
{
	/* 索引区首地址 */
	ee_uint32 indexStartAddr;
	/* 索引重写区首地址 */
	ee_uint32 overwriteAddr;
	/* 索引交换区首地址 */
	ee_uint32 indexSwapStartAddr;
	/* 数据区首地址 */
	ee_uint32 dataStartAddr;
	/* 数据交换区首地址 */
	ee_uint32 dataSwapStartAddr;
	/* 数据索引区总大小(单位:扇区) */
	ee_uint16 indexRegionSize;
	/* 索引区总大小(单位:扇区) */
	ee_uint16 indexAreaSize;
	/* 数据区总大小(单位:扇区) */
	ee_uint16 dataRegionSize;
	/* 索引重写计数区总大小(单位:字节) */
	ee_uint16 overwriteCountAreaSize;
}flash_MemMang_t;


/* 想保存变量到flash时，首先在下面枚举中添加变量名 */
typedef enum
{
	// DATA0,DATA1只是示例，实际使用将70-74行全部删除
	DATA0,
	DATA1,
	// 用户将变量名添加到下面
	G_MYSENSORDATA,

	// DATA_NUM用于标识flash中一共存了多少个数据(不允许删改)
	DATA_NUM,
}variableLists;

void ee_flashInit(flash_MemMang_t* pobj,ee_uint32 indexStartAddr,ee_uint32 indexSwapStartAddr,ee_uint16 indexRegionSize,ee_uint16 indexSize,ee_uint32 dataStartAddr,ee_uint32 dataSwapStartAddr,ee_uint16 dataRegionSize);
ee_uint8 ee_readDataFromFlash(flash_MemMang_t* pobj, void* buf, variableLists dataId);
ee_uint8 ee_writeDataToFlash(flash_MemMang_t* pobj, void* buf, ee_uint16 bufSize, variableLists dataId);
#endif /* __FLASH_MEMMANG_H_ */

