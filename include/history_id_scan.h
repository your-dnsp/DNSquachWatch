#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
namespace ReadableLogs {
// A complete duplicate search, bounded to one 1 KiB read per UI iteration.
// Keeping a suffix catches IDs split across reads without heap allocation.
struct IdScan {
 enum Result{WAIT,FOUND,ABSENT,ERROR};
 char tail[32]{};size_t keep=0;uint32_t bytes=0;
 void reset(){keep=0;bytes=0;}
 template<class Reader> Result tick(Reader& reader,const char* id){
  size_t want=strlen(id);if(!want||want>=sizeof tail)return ERROR;
  if(!reader.available())return ABSENT;
  char buf[1024+32+1];memcpy(buf,tail,keep);
  int n=reader.read((uint8_t*)buf+keep,1024);if(n<=0||n>1024)return ERROR;
  size_t used=keep+size_t(n);buf[used]=0;bytes+=uint32_t(n);
  if(strstr(buf,id))return FOUND;
  keep=used<want-1?used:want-1;memcpy(tail,buf+used-keep,keep);
  return reader.available()?WAIT:ABSENT;
 }
};
}
