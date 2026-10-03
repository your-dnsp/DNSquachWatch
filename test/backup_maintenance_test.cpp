#include "backup_maintenance.h"
#include "content_contract.h"
#include <SD.h>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <unistd.h>
void make(unsigned i,const char* version){std::string root=TestSD::root+"/dnsp-backup-"+std::to_string(i);std::filesystem::create_directory(root);std::ofstream(root+"/firmware.bin")<<"sample";if(version)std::ofstream(root+"/COMPLETE.txt")<<"DNSP_BACKUP_V2\nfirmware=DNSquachWatch "<<version<<"\n";}
void run(){for(unsigned i=0;i<200&&BackupMaintenance::busy();i++)BackupMaintenance::tick();assert(!BackupMaintenance::busy());}
int main(){char temp[]="/tmp/dnsp-maintenance-XXXXXX";assert(mkdtemp(temp));TestSD::root=temp;make(0,"v1.5.1");make(1,"v1.5.7");make(2,nullptr);make(3,"v1.6.0");assert(BackupMaintenance::start(false,true));run();assert(!SD.exists("/dnsp-backup-2"));assert(SD.exists("/dnsp-backup-0/COMPLETE.txt"));assert(BackupMaintenance::start(true,true));run();assert(!SD.exists("/dnsp-backup-0"));assert(SD.exists("/Archive/dnsp-backup-0-0000/COMPLETE.txt"));assert(SD.exists("/dnsp-backup-1")&&SD.exists("/dnsp-backup-3"));make(0,"v1.5.1");assert(BackupMaintenance::start(true,true));run();assert(SD.exists("/Archive/dnsp-backup-0-0001/COMPLETE.txt"));make(2,nullptr);assert(BackupMaintenance::start(false,true));BackupMaintenance::tick();BackupMaintenance::cancel();assert(!BackupMaintenance::busy());assert(TestSD::openHandles==0);assert(!BackupMaintenance::start(false,false));
 assert(ContentContract::supported("{\"required_content\":\"v1.5\"}"));assert(!ContentContract::supported("{}"));assert(!ContentContract::supported("{\"required_content\":\"v1.6\"}"));assert(!ContentContract::supported("{\"required_content\":\"v1.5\",\"required_content\":\"v1.6\"}"));assert(!ContentContract::supported("{\"required_content\" \"v1.5\"}"));unsigned char digest[32];
 assert(ContentContract::imageDigest("{\"signed_image_sha256\":\"" "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef" "\"}",digest));assert(digest[0]==1&&digest[31]==0xef);
 assert(!ContentContract::imageDigest("{}",digest));assert(!ContentContract::imageDigest("{\"signed_image_sha256\":\"01\"}",digest));
 assert(!ContentContract::imageDigest("{\"signed_image_sha256\":\"",digest));
 std::filesystem::remove_all(temp);puts("Backup maintenance and OTA content contract PASS");}
