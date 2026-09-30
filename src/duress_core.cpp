#include "duress_core.h"
#include <string.h>
#include <stdio.h>
namespace Duress {
namespace {
struct Record { uint32_t magic, value, inverse, checksum; };
constexpr uint32_t INTENT=0x44555039, COMPLETE=0x54494439;
uint32_t checksum(const Record& r) {
    const uint32_t words[]={r.magic,r.value,r.inverse};
    uint32_t h=2166136261u;
    for (uint32_t w:words) for(unsigned b=0;b<4;b++) {h^=(w>>(8*b))&255;h*=16777619u;}
    return h;
}
Record record(uint32_t magic,uint32_t value) {
    Record r{magic,value,~value,0}; r.checksum=checksum(r); return r;
}
bool valid(const Record& r,uint32_t magic) {
    return r.magic==magic && r.inverse==~r.value && r.checksum==checksum(r);
}
bool write(Storage& s,unsigned offset,const Record& r) {
    Record got{};
    return s.writeJournal(offset,&r,sizeof r) && s.readJournal(offset,&got,sizeof got)
        && !memcmp(&r,&got,sizeof r);
}
}
Boot inspect(Storage& s,uint32_t* errors) {
    Record intent{},end{};
    if(errors)*errors=0;
    if(!s.readJournal(0,&intent,sizeof intent))return Boot::FAULT;
    if(!valid(intent,INTENT) || intent.value!=1) {
        // A recognizable but torn intent never returns to ordinary operation.
        return intent.magic==INTENT ? Boot::FAULT : Boot::NORMAL;
    }
    if(!s.readJournal(sizeof intent,&end,sizeof end))return Boot::FAULT;
    if(valid(end,COMPLETE)) {if(errors)*errors=end.value;return Boot::DECOY;}
    return Boot::PENDING;
}
bool arm(Storage& s) {
    // Never erase a committed intent (including a partially written completion).
    if(inspect(s)!=Boot::NORMAL)return false;
    return s.eraseJournal() && write(s,0,record(INTENT,1));
}
bool safeLeaf(const char* n) {
    return n && *n && strcmp(n,".") && strcmp(n,"..") && !strchr(n,'/') && !strchr(n,'\\');
}
bool appName(const char* n) {
    return safeLeaf(n) && (!strcmp(n,"crash-reports") || !strcmp(n,"Device Health Export") ||
                           !strncmp(n,"dnsp-",5) || !strncmp(n,"squachwatch-",12));
}
void Wipe::begin(uint32_t now) {
    *this=Wipe{};started=now;strcpy(stack[0].path,"/");
}
void Wipe::finish(Storage& s) {
    if(mounted && !s.unmountCard())failures|=SD_IO;
    mounted=false;
    if(!write(s,sizeof(Record),record(COMPLETE,failures)))failures|=JOURNAL;
    // A failed completion remains PENDING on flash; next boot retries safely.
    finished=true;
}
void Wipe::tick(Storage& s,uint32_t now) {
    if(finished)return;
    if(!intentChecked){
        if(inspect(s)!=Boot::PENDING){failures|=JOURNAL;finished=true;return;}
        intentChecked=true;
    }
    static const unsigned counts[]={5,32,15};
    if(region<3) {
        if(!s.eraseBlock(region,sector))failures|=1u<<region;
        if(++sector==counts[region]){sector=0;region++;}
        return;
    }
    if(!cardStarted){
        cardStarted=true;mounted=s.mountCard();
        if(!mounted){failures|=SD_IO;finish(s);}return;
    }
    // Bound work on corrupt/huge cards. This is explicitly a partial quick wipe.
    if((uint32_t)(now-started)>7500 || visited++>=2048){failures|=SD_LIMIT;finish(s);return;}
    Frame& f=stack[depth];Node node{};
    int result=s.list(f.path,f.index,node);
    if(result<=0){
        if(result<0)failures|=SD_IO;
        if(!depth){finish(s);return;}
        bool removed=s.eraseDirectory(f.path);depth--;
        if(!removed){failures|=SD_IO;stack[depth].index++;}
        return;
    }
    node.name[sizeof(node.name)-1]=0;
    if(!safeLeaf(node.name)){failures|=SD_IO;f.index++;return;}
    if(!depth && !appName(node.name)){f.index++;return;}
    char path[256];int n=snprintf(path,sizeof path,"%s%s%s",f.path,depth?"/":"",node.name);
    if(n<0 || (size_t)n>=sizeof path){failures|=SD_LIMIT;f.index++;return;}
    if(node.directory){
        if(depth==8){failures|=SD_LIMIT;f.index++;return;}
        depth++;strcpy(stack[depth].path,path);stack[depth].index=0;
    } else if(!s.eraseFile(path)){failures|=SD_IO;f.index++;}
}
}
