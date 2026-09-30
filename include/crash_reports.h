#pragma once
#include "ui_diagnostics.h"
namespace CrashReports {
struct Record {
    uint32_t sequence=0, resetReason=0;
    CrashReport crash={};
    uint16_t lightReading=200;
    uint8_t backlightDuty=0, displayMhz=80;
    bool ldr=false;
    char firmware[24]={};
};
bool enqueue(const Record& record); // post-reset only; bounded 8-record NVS queue
void service(bool ready,uint32_t now);
const char* status();
const char* root(); // /sd on device, explicit temporary directory in tests
bool backupPresent(bool ready); // completed backup marker + firmware; not a new hash verification
}
