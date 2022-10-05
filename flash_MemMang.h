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

/* 当前flash一个扇区的大小 */
#define SECTOR_SIZE 4096
/* 当前flash一个块有多少个扇区 */
#define BLOCk_SECTOR_NUM 

/* 返回第x扇区的地址 */
#define SECTORS(x) ((x) * SECTOR_SIZE)
/* 返回第x块的地址 */
#define BLOCKS(x)  ((x) * BLOCk_SECTOR_NUM * SECTOR_SIZE)

/* 函数类型 void (*) (uint32 flashAddr, uint8* dataAddr, uint16 num) */
#define flashWirte 
#define flashRead  

/* 函数原型 void (*) (uint32_t flashAddr) */
#define flashEraseASector  

typedef struct
{
	/* 数据索引区首地址 */
	ee_uint32 IndexStartAddr;
	/* 数据索引交换区首地址 */
	ee_uint32 IndexSwapStartAddr;
	/* 数据区首地址 */
	ee_uint32 dataStartAddr;
	/* 数据交换区首地址 */
	ee_uint32 dataSwapStartAddr;
	/* 数据索引区总大小(单位:扇区) */
	ee_uint16 IndexRegionSize;
	/* 数据区总大小(单位:扇区) */
	ee_uint16 dataRegionSize;
}flash_MemMang_t;


/* 想保存变量到flash时，首先在下面枚举中添加变量名 */
typedef enum
{
	// DATA0,DATA1只是示例，实际使用将17-21行全部删除
	DATA0,
	DATA1,
	// 用户将变量名添加到下面
	G_MYSENSORDATA,
	
}variableLists;

ee_uint8 readDataFromFlash(flash_MemMang_t* pobj, void* dataAddr, variableLists dataId);
ee_uint8 writeDataToFlash(flash_MemMang_t* pobj, void* dataAddr, ee_uint16 dataSize, variableLists dataId);
#endif /* __FLASH_MEMMANG_H_ */

