#pragma once
#include <cstdint>
#include <cstdlib>
using DWORD=uint32_t;
using FRESULT=int;
constexpr int FR_OK=0,FR_INVALID_DRIVE=11,FM_ANY=0,FS_FAT12=1,FS_FAT16=2,FS_FAT32=3;
struct FATFS {uint32_t n_fatent=2048;uint8_t fs_type=FS_FAT32;};
namespace FatTest {inline int result=FR_OK;inline FATFS volume;}
inline FRESULT f_getfree(const char*,DWORD* count,FATFS** fs){*count=512;*fs=&FatTest::volume;return FatTest::result;}
inline FRESULT f_mount(FATFS*,const char*,int){return FR_OK;}
inline FRESULT f_mkfs(const char*,int,int,void*,int){return FR_OK;}
inline void* heap_caps_malloc(size_t n,int){return malloc(n);}
