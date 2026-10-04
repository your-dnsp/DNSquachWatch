#pragma once
#include <SPI.h>
inline void spiAttachSCK(void*,int p){BusTest::sck=p;}
inline void spiAttachMOSI(void*,int p){BusTest::mosi=p;}
inline void spiAttachMISO(void*,int p){BusTest::miso=p;}
