#pragma once
#include <stddef.h>
#include <stdint.h>
#include "ui_settings.h"
namespace Care {
constexpr size_t VALUE_COUNT=32, SETTINGS_CAP=1024;
struct Snapshot { uint32_t values[VALUE_COUNT]{}; uint8_t language=0,hebrew=0,contrast=0,reduced=0,left=0,large=0; };
uint32_t crc(const void*,size_t);
bool valid(const Snapshot&);
bool encode(const Snapshot&,char*,size_t);
bool decode(const char*,size_t,Snapshot&);
void capture(Snapshot&);
bool apply(const Snapshot&); // validates whole input before any preferences write
void begin();
SettingsRow favorite(uint8_t);
const char* favoriteName(uint8_t);
void cycleFavorite(uint8_t);
void armGift(bool);
bool giftArmed();
bool giftDue();
void consumeGift();
struct Health { uint32_t loops=0,maxGap=0,minHeap=UINT32_MAX,minBlock=UINT32_MAX,over250=0; };
void noteLoop(uint32_t now,uint32_t heap,uint32_t block);
Health health();
bool bootReady(uint32_t now);
}
namespace Backup {
bool start(bool card,uint32_t now);
void tick();
void cancel();
bool busy();
const char* status();
unsigned percent();
uint8_t slot();
void nextSlot();
bool restore(bool card);
bool verifiedThisBoot();
bool exportHealth(bool card,const char* report);
}
