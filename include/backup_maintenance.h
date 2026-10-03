#pragma once
namespace BackupMaintenance {
bool start(bool archiveOlder, bool mounted);
void tick();
void cancel();
bool busy();
const char* status();
}
