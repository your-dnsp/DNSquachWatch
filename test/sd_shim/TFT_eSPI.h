#pragma once
#include <SPI.h>
class TFT_eSPI {public: SPIClass spi; SPIClass& getSPIinstance(){return spi;}};
