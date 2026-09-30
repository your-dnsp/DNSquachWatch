#include "duress_device.h"
#include <string.h>
#if __has_include(<esp_flash.h>)
#include <esp_flash.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>
#include <nvs_flash.h>
#include <SD.h>
#include <SPI.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include "ff.h"
#include "diskio_impl.h"
#include "diskio.h"
namespace {
const esp_partition_t* journal() {
    auto p=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,(esp_partition_subtype_t)0x40,"duress");
    return p && p->address==0x3ff000 && p->size==4096 ? p : nullptr;
}
bool partition(const char* name,uint32_t address,uint32_t size) {
    auto p=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_ANY,name);
    return p && p->address==address && p->size==size;
}
bool layout() {
    if(!journal() || !partition("nvs",0x9000,0x5000) || !partition("coredump",0x3f0000,0xf000))return false;
    // BlackBox is intentionally outside the partition table. Reject any overlap.
    auto it=esp_partition_find(ESP_PARTITION_TYPE_ANY,ESP_PARTITION_SUBTYPE_ANY,nullptr);
    while(it){auto p=esp_partition_get(it);if(p->address<0x3f0000 && p->address+p->size>0x3d0000){esp_partition_iterator_release(it);return false;}it=esp_partition_next(it);}
    return true;
}
class Device final:public Duress::Storage {
    uint8_t drive=255;
public:
    bool readJournal(uint32_t o,void* b,size_t n) override {
        auto p=journal();return p && o<=4096 && n<=4096-o && esp_partition_read(p,o,b,n)==ESP_OK;
    }
    bool eraseJournal() override {auto p=journal();return p && esp_partition_erase_range(p,0,4096)==ESP_OK;}
    bool writeJournal(uint32_t o,const void* b,size_t n) override {
        auto p=journal();return p && o<=4096 && n<=4096-o && esp_partition_write(p,o,b,n)==ESP_OK;
    }
    bool eraseBlock(unsigned region,unsigned sector) override {
        static const uint32_t bases[]={0x9000,0x3d0000,0x3f0000};
        static const unsigned sizes[]={5,32,15};
        if(region>=3 || sector>=sizes[region] || !layout())return false;
        if(region==0 && sector==0){
            esp_err_t e=nvs_flash_deinit();
            if(e!=ESP_OK && e!=ESP_ERR_NVS_NOT_INITIALIZED)return false;
        }
        uint32_t address=bases[region]+4096*sector;
        if(esp_flash_erase_region(esp_flash_default_chip,address,4096)!=ESP_OK)return false;
        uint8_t b[256];
        for(unsigned o=0;o<4096;o+=sizeof b){
            if(esp_flash_read(esp_flash_default_chip,b,address+o,sizeof b)!=ESP_OK)return false;
            for(uint8_t v:b)if(v!=255)return false;
        }
        return true;
    }
    bool mountCard() override {
        ff_diskio_get_drive(&drive);
        SPI.begin(18,19,23,5);
        if(SD.begin(5,SPI,4000000,"/sd",2))return true;
        SD.end();return false;
    }
    int list(const char* path,unsigned index,Duress::Node& out) override {
        // Arduino File::openNextFile conflates end-of-directory with an
        // allocation/open error. Use the mounted VFS directory API so a read
        // error cannot be reported as a clean end of the wipe.
        char mountedPath[264];int n=snprintf(mountedPath,sizeof mountedPath,"/sd%s",path);
        if(n<0 || (size_t)n>=sizeof mountedPath)return -1;
        DIR* dir=opendir(mountedPath);if(!dir)return -1;
        int result=0;
        for(;;){
            errno=0;dirent* entry=readdir(dir);
            if(!entry){result=errno?-1:0;break;}
            if(!strcmp(entry->d_name,".") || !strcmp(entry->d_name,".."))continue;
            if(index){--index;continue;}
            if(strlen(entry->d_name)>=sizeof out.name ||
               (entry->d_type!=DT_DIR && entry->d_type!=DT_REG)){result=-1;break;}
            strcpy(out.name,entry->d_name);out.directory=entry->d_type==DT_DIR;result=1;break;
        }
        if(closedir(dir)!=0)result=-1;
        return result;
    }

    bool eraseFile(const char* path) override {
        uint8_t zero[512]{};
        File f=SD.open(path,"r+");if(!f || f.isDirectory()){f.close();return false;}
        size_t n=f.size();if(n>sizeof zero)n=sizeof zero;
        bool ok=!n || (f.seek(0) && f.write(zero,n)==n);
        f.flush();f.close();
        if(!ok || drive>=10 || disk_ioctl(drive,CTRL_SYNC,nullptr)!=RES_OK)return false;
        f=SD.open(path,FILE_READ);if(!f)return false;
        uint8_t read[512];ok=f.read(read,n)==(int)n;f.close();
        if(!ok)return false;
        for(size_t i=0;i<n;i++)if(read[i])return false;
        // On failure leave the entry in place so the walk cannot skip its sibling.
        return SD.remove(path) && !SD.exists(path);
    }
    bool eraseDirectory(const char* path) override {return SD.rmdir(path) && !SD.exists(path);}
    bool unmountCard() override {
        bool ok=drive<10 && disk_ioctl(drive,CTRL_SYNC,nullptr)==RES_OK;
        SD.end();drive=255;return ok;
    }
};
Device device;
}
#else
// Desktop simulation never writes real flash or a host filesystem. Tests use
// the same policy engine with an injected, failure-capable Storage backend.
namespace {
class Device final:public Duress::Storage {
    uint8_t bytes[4096];
public:
    Device(){memset(bytes,255,sizeof bytes);}
    bool readJournal(uint32_t o,void* b,size_t n)override {if(o>4096||n>4096-o)return false;memcpy(b,bytes+o,n);return true;}
    bool eraseJournal()override {memset(bytes,255,sizeof bytes);return true;}
    bool writeJournal(uint32_t o,const void* b,size_t n)override {if(o>4096||n>4096-o)return false;auto p=(const uint8_t*)b;for(size_t i=0;i<n;i++)bytes[o+i]&=p[i];return true;}
    bool eraseBlock(unsigned,unsigned)override{return true;}
    bool mountCard()override{return true;}
    int list(const char*,unsigned,Duress::Node&)override{return 0;}
    bool eraseFile(const char*)override{return true;}
    bool eraseDirectory(const char*)override{return true;}
    bool unmountCard()override{return true;}
};
Device device;
bool layout(){return true;}
}
#endif
namespace DuressDevice {
static Duress::Wipe wipe;
bool available(){return layout();}
Duress::Boot boot(){
    // No journal is the normal v0.8 app-only migration case. Do not erase data.
#if __has_include(<esp_flash.h>)
    if(!journal())return Duress::Boot::NORMAL;
#endif
    auto b=Duress::inspect(device);
    return b!=Duress::Boot::NORMAL && !layout()?Duress::Boot::FAULT:b;
}
bool arm(){
    if(!available())return false;
#if __has_include(<esp_flash.h>)
    // Do not reboot a probationary OTA image into an older application that
    // may not understand this journal. This does not start an update service.
    auto running=esp_ota_get_running_partition();if(!running)return false;
    esp_ota_img_states_t state;
    esp_err_t status=esp_ota_get_state_partition(running,&state);
    if(status!=ESP_OK && status!=ESP_ERR_NOT_FOUND)return false;
    if(status==ESP_OK && state==ESP_OTA_IMG_PENDING_VERIFY &&
       esp_ota_mark_app_valid_cancel_rollback()!=ESP_OK)return false;
#endif
    return Duress::arm(device);
}
void begin(uint32_t now){wipe.begin(now);}
void tick(uint32_t now){wipe.tick(device,now);}
bool done(){return wipe.done();}
uint32_t errors(){return wipe.errors();}
}
