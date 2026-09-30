#pragma once
#include <stdint.h>
namespace ReadableLogs {
// BACKUP_INTERNAL refreshes only records held inside the ESP.  A backup is
// already being written to the same card, so copying the card's root logs
// back onto itself would waste time and space without protecting anything.
enum class Mode:uint8_t{REFRESH,SNAPSHOT,BACKUP_INTERNAL};
bool start(Mode);
void tick();
void cancel();
bool busy();
bool succeeded();
unsigned percent();
const char* phase();
const char* status();
const char* outputPath();
}
