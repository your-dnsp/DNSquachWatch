#pragma once
constexpr int CTRL_SYNC=0,RES_OK=0;
inline int disk_ioctl(uint8_t,int,void*){return RES_OK;}
