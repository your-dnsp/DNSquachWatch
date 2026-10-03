#pragma once
#include <stdint.h>
namespace ReadableLogs {
// BACKUP_INTERNAL refreshes only records held inside the ESP.  A backup is
// already being written to the same card, so copying the card's root logs
// back onto itself would waste time and space without protecting anything.
enum class Mode:uint8_t{REFRESH,SNAPSHOT,BACKUP_INTERNAL};
bool start(Mode);
bool automatic();
void automaticTick(uint32_t now,bool available);
void tick();
void cancel();
bool busy();
bool succeeded();
unsigned percent();
uint32_t progressToken();
const char* phase();
const char* status();
const char* outputPath();
}
