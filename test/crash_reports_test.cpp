#include "test_util.h"
#include "crash_reports.h"
#include <Preferences.h>
#include <esp_heap_caps.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <cstdlib>
#include <unistd.h>
int main(){
 char temp[]="/tmp/dnsp-crash-test-XXXXXX";char* root=mkdtemp(temp);if(!root)return 2;
 setenv("DNSP_TEST_SD",root,1);setenv("SQUACHSIM_NVS",root,1);
 namespace fs=std::filesystem;
 auto files=[&](){size_t n=0;if(fs::is_directory(fs::path(root)/"crash-reports"))for(auto& p:fs::directory_iterator(fs::path(root)/"crash-reports"))n+=p.path().extension()==".txt";return n;};
 suite("Crash export persistence and failures");
 CrashReports::Record r;r.resetReason=4;r.crash.valid=true;r.crash.pc=0x12345678;snprintf(r.firmware,sizeof r.firmware,"DNSP test");
 ck("queue crash",CrashReports::enqueue(r));CrashReports::service(false,6000);ck("absent card does not export",files()==0);
 {std::ofstream bad(fs::path(root)/"crash-reports");bad<<"blocked";}
 CrashReports::service(true,12000);ck("bad destination reported",strstr(CrashReports::status(),"cannot create")!=nullptr);fs::remove(fs::path(root)/"crash-reports");
 CrashReports::service(true,18000);ck("retry exports preserved report",files()==1);
 std::string text;for(auto& p:fs::directory_iterator(fs::path(root)/"crash-reports")){std::ifstream f(p.path());text.assign(std::istreambuf_iterator<char>(f),{});}
 ck("report contains fault and firmware",text.find("12345678")!=std::string::npos&&text.find("DNSP test")!=std::string::npos);
 CrashReports::service(true,24000);ck("successful report not duplicated",files()==1);
 bool all=true;for(int i=0;i<8;++i){r.crash.pc++;all&=CrashReports::enqueue(r);}ck("eight pending reports fit",all);
 ck("full queue refuses to overwrite pending evidence",!CrashReports::enqueue(r));
 for(int i=0;i<9;++i)CrashReports::service(true,30000+6000*i);
 ck("individual files for every queued crash",files()==9);
 ck("no backup claimed on ordinary card",!CrashReports::backupPresent(true));
 fs::create_directory(fs::path(root)/"dnsp-backup-0");{std::ofstream f(fs::path(root)/"dnsp-backup-0/firmware.bin");f<<"image";}
 ck("incomplete backup not advertised",!CrashReports::backupPresent(true));
 {std::ofstream f(fs::path(root)/"dnsp-backup-0/COMPLETE.txt");f<<"verified marker";}
 ck("completed backup found",CrashReports::backupPresent(true));ck("unmounted backup not advertised",!CrashReports::backupPresent(false));
 fs::remove_all(root);return report();
}
