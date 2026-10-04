#pragma once
#include <cstdint>
namespace BusTest {inline unsigned begins=0;inline int sck=-1,mosi=-1,miso=-1;}
class SPIClass {bool started=false;public: void begin(int s=-1,int i=-1,int o=-1,int=-1){if(!started){++BusTest::begins;started=true;BusTest::sck=s;BusTest::miso=i;BusTest::mosi=o;}} void* bus(){return this;}};
inline SPIClass SPI;
