#include "test_util.h"
#include "duress_core.h"
#include "pixel_tide.h"
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
using namespace Duress;
struct Fake:Storage {
    uint8_t journal[4096];
    std::map<std::string,bool> files;
    std::vector<std::string> removed;
    unsigned blocks=0,mounts=0,unmounts=0;
    int cut=-1; bool readFail=false,eraseFail=false,blockFail=false,mountFail=false,syncFail=false,deleteFail=false,listFail=false;
    Fake(){memset(journal,255,sizeof journal);}
    bool readJournal(uint32_t o,void* b,size_t n)override{if(readFail)return false;memcpy(b,journal+o,n);return true;}
    bool eraseJournal()override{if(eraseFail)return false;memset(journal,255,sizeof journal);return true;}
    bool writeJournal(uint32_t o,const void* b,size_t n)override{
        auto p=(const uint8_t*)b;size_t count=cut<0?n:std::min(n,(size_t)cut);
        for(size_t i=0;i<count;i++)journal[o+i]&=p[i];return count==n;
    }
    bool eraseBlock(unsigned,unsigned)override{blocks++;return !blockFail;}
    bool mountCard()override{mounts++;return !mountFail;}
    int list(const char* p,unsigned index,Node& n)override{
        if(listFail)return -1;
        std::string prefix=p;if(prefix!="/")prefix+="/";
        for(auto& f:files){if(f.first.rfind(prefix,0))continue;
            auto leaf=f.first.substr(prefix.size());if(leaf.empty()||leaf.find('/')!=std::string::npos)continue;
            if(index--==0){strncpy(n.name,leaf.c_str(),sizeof n.name-1);n.directory=f.second;return 1;}
        }return 0;
    }
    bool eraseFile(const char* p)override{if(deleteFail)return false;removed.emplace_back(p);return files.erase(p)==1;}
    bool eraseDirectory(const char* p)override{
        for(auto& f:files)if(f.first.rfind(std::string(p)+"/",0)==0)return false;
        return files.erase(p)==1;
    }
    bool unmountCard()override{unmounts++;return !syncFail;}
};
void run(Wipe& w,Fake& f,uint32_t base=0){for(unsigned i=0;i<2300&&!w.done();i++)w.tick(f,base+i);}
int main(){
    suite("Durable intent precedes every erase");
    Fake f;ck("empty journal starts ordinary firmware",inspect(f)==Boot::NORMAL);
    Wipe unarmed;unarmed.begin(0);run(unarmed,f);ck("unarmed wipe refuses all erases",f.blocks==0 && f.mounts==0 && unarmed.errors()==JOURNAL);
    f.eraseFail=true;ck("failed journal erase cannot arm",!arm(f));f.eraseFail=false;
    for(int cut=0;cut<16;cut++){Fake interrupted;interrupted.cut=cut;ck("torn intent is never reported as armed",!arm(interrupted) && interrupted.blocks==0);}
    ck("intent commits",arm(f)&&inspect(f)==Boot::PENDING);
    ck("cannot overwrite pending intent",!arm(f));
    f.files={{"/crash-reports",true},{"/crash-reports/crash-test.txt",false},{"/dnsp-backup-0",true},{"/dnsp-backup-0/firmware.bin",false},{"/dnsp-backup-0/nested",true},{"/dnsp-backup-0/nested/settings",false},{"/dnsp-report.csv",false},{"/squachwatch-today.log",false},{"/photo.jpg",false},{"/personal",true},{"/personal/dnsp-keep.txt",false}};
    Wipe w;w.begin(0);run(w,f);
    ck("all flash blocks attempted",f.blocks==52);
    ck("owned directories recursively removed",f.files.size()==3 && f.files.count("/photo.jpg") && f.files.count("/personal/dnsp-keep.txt"));
    ck("card unmounted exactly once",f.mounts==1 && f.unmounts==1);
    ck("clean completion persists",w.done() && !w.errors() && inspect(f)==Boot::DECOY);
    auto blocks=f.blocks;w.tick(f,9000);ck("finished wipe does no further work",f.blocks==blocks);
    suite("Power interruption, failures and bounds");
    bool all=true;
    for(unsigned cut=0;cut<56;cut++){
        Fake reboot;arm(reboot);Wipe first;first.begin(0);
        for(unsigned i=0;i<cut&&!first.done();i++)first.tick(reboot,i);
        auto boot=inspect(reboot);if(boot==Boot::PENDING){Wipe resumed;resumed.begin(0);run(resumed,reboot);}
        all &= inspect(reboot)==Boot::DECOY;
    }ck("restart at every flash/card step reaches decoy",all);
    Fake bad;arm(bad);bad.blockFail=true;bad.mountFail=true;Wipe failed;failed.begin(0);run(failed,bad);uint32_t errors=0;
    ck("all flash failures and missing SD are recorded",failed.errors()==(NVS|BLACKBOX|COREDUMP|SD_IO) && inspect(bad,&errors)==Boot::DECOY && errors==failed.errors());
    Fake full;arm(full);full.files["/dnsp-log"]=false;full.deleteFail=true;full.syncFail=true;Wipe noDelete;noDelete.begin(0);run(noDelete,full);
    ck("failed deletion is bounded and recorded",noDelete.done() && (noDelete.errors()&SD_IO) && full.files.size()==1);
    Fake dirError;arm(dirError);dirError.listFail=true;Wipe unread;unread.begin(0);run(unread,dirError);
    ck("directory read failure is not a clean end",unread.done() && (unread.errors()&SD_IO));
    Fake timeout;arm(timeout);Wipe timed;timed.begin(0xfffffff0u);
    for(unsigned i=0;i<53;i++)timed.tick(timeout,0xfffffff0u+i);
    timed.tick(timeout,0xfffffff0u+8000);ck("elapsed timeout handles clock wrap",timed.done() && (timed.errors()&SD_LIMIT));
    Fake writeFail;arm(writeFail);writeFail.cut=3;Wipe endFail;endFail.begin(0);run(endFail,writeFail);
    ck("torn completion stays pending",endFail.done() && (endFail.errors()&JOURNAL) && inspect(writeFail)==Boot::PENDING);
    writeFail.cut=-1;Wipe retry;retry.begin(0);run(retry,writeFail);ck("interrupted completion can be retried",inspect(writeFail)==Boot::DECOY);
    Fake unreadable;unreadable.readFail=true;ck("unreadable journal is fail-closed",inspect(unreadable)==Boot::FAULT && !arm(unreadable));
    ck("unsafe leaves and non-app roots rejected",!safeLeaf("..")&&!safeLeaf("a/b")&&!safeLeaf("a\\b")&&!appName("photos")&&appName("dnsp-backup-0")&&appName("crash-reports")&&!appName("crash-reports-personal"));
    Fake deep;arm(deep);std::string path="/dnsp-deep";deep.files[path]=true;for(int i=0;i<12;i++){path+="/x";deep.files[path]=true;}Wipe bounded;bounded.begin(0);run(bounded,deep);
    ck("deep directory bounded and partial recorded",bounded.done() && (bounded.errors()&SD_LIMIT));
    suite("Fixed-memory original PIXEL TIDE");
    PixelTide::reset();auto before=PixelTide::color(20,13);PixelTide::touch(160,91,320,182);ck("touch changes ripple field",PixelTide::color(20,13)!=before);
    for(int i=0;i<20000;i++){PixelTide::touch(i%400-40,i%250-30,320,182);PixelTide::step();}
    ck("out-of-range pixel harmless",PixelTide::color(999,999)==0);
    return report();
}
