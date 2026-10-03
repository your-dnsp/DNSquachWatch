#pragma once
#include <stdint.h>
namespace ReadableLogs {
// BACKUP_INTERNAL refreshes only records held inside the ESP.  A backup is
// already being written to the same card, so copying the card's root logs
// back onto itself would waste time and space without protecting anything.
enum class Mode:uint8_t{REFRESH,SNAPSHOT,BACKUP_INTERNAL};
bool start(Mode);
bool automatic();
// mayWork pauses background refresh for touch/alerts without cancelling its journal.
void automaticTick(uint32_t now,bool available,bool mayWork=true);
void tick();
void cancel();
bool busy();
bool succeeded();
unsigned percent();
uint32_t progressToken();
// Slowest individual card operation in the most recent processing step.
const char* operation();
uint32_t operationMicros();
const char* step();
const char* phase();
const char* status();
const char* outputPath();
}
