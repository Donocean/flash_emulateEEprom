/**
 * @file spi_flash_MemMang.h
 * @author Donocean (1184427366@qq.com)
 * @brief 用于 flash 的内存管理
 * @version 1.0
 * @date 2022-10-20
 * 
 * @copyright Copyright (c) 2022, Donocean
 */

#ifndef __FLASH_MEMMANG_H_
#define __FLASH_MEMMANG_H_

/* spi flash 一个block(块)有16个sector(扇区)
 * 一个扇区4KB，那么一个块就为64KB 
 * flash 写入只能将1变成0，所以在写入重复的存储区域时，
 * 需要先将重复的区域擦除，flash最小的擦除单元为sector(扇区)
 * 此程序抽象了一个上述过程，使你可以使用此程序驱动spi_flash或其他芯片的片内flash */

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

/* 函数原型 void (*) (uint32 flashAddr) */
#define ee_flashEraseASector 

/* 用户不要修改结构体中的任何成员 */
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
	// DATA0,DATA1,G_MYSENSORDATA只是示例，实际使用将71-75行全部删除
	DATA0,
	DATA1,
	// 用户将变量名添加到下面
	G_MYSENSORDATA,

	// DATA_NUM用于标识flash中一共存了多少个数据(不允许删改)
	DATA_NUM,
}variableLists;

/**
 * @brief                      格式化传入函数地址flash
 *
 * @param pobj                 flash管理对象指针
 * @param indexStartAddr       总索引区起始地址
 * @param indexSwapStartAddr   交换总索引区起始地址
 * @param indexRegionSize      总索引区大小(单位：扇区)
 * @param indexSize            索引区大小(详见README图例，要小于indexRegionSize)
 * @param dataStartAddr        数据区起始地址
 * @param dataSwapStartAddr    交换数据区起始地址
 * @param dataRegionSize       数据区大小(单位：扇区)
 */
void ee_flashInit(flash_MemMang_t* pobj,ee_uint32 indexStartAddr,ee_uint32 indexSwapStartAddr,ee_uint16 indexRegionSize,ee_uint16 indexSize,ee_uint32 dataStartAddr,ee_uint32 dataSwapStartAddr,ee_uint16 dataRegionSize);

/**
 * @brief           从flash读取数据
 *
 * @param pobj      flash管理对象指针
 * @param buf       读取数据的地址
 * @param dataId    要读取的数据id(详见头文件枚举类型variableLists)
 *
 * @retval 
 *         0: 读取成功
 *         1: 读取的数据超过索引区
 *         2: 当前读取的数据id没有写入过
 *         3: 当前读取的数据id不是有效的
 */
ee_uint8 ee_readDataFromFlash(flash_MemMang_t* pobj, void* buf, variableLists dataId);

/**
 * @brief           写数据到flash
 *
 * @param pobj      flash管理对象指针
 * @param buf       写入数据的地址
 * @param bufSize   数据大小
 * @param dataId    要写入的数据id(详见头文件枚举类型variableLists)
 *
 * @retval 
 *         0: 写入成功
 *         1: 写入的数据超过索引区
 *         2: 当前写入的数据id，没有遵循variableLists中的顺序写入
 */
ee_uint8 ee_writeDataToFlash(flash_MemMang_t* pobj, void* buf, ee_uint16 bufSize, variableLists dataId);

#endif /* __FLASH_MEMMANG_H_ */

