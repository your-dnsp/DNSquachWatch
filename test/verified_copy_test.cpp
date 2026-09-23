#include "test_util.h"
#include "verified_copy.h"
#include <vector>
#include <algorithm>
#include <string>
struct Source {std::vector<uint8_t> bytes;bool fail=false;bool read(uint32_t at,uint8_t* p,size_t n){if(fail||at+n>bytes.size())return false;memcpy(p,bytes.data()+at,n);return true;}};
struct Dest {std::vector<uint8_t> bytes;size_t at=0;bool failWrite=false,failRead=false,corrupt=false,badLength=false;
 bool write(const uint8_t* p,size_t n){if(failWrite)return false;bytes.insert(bytes.end(),p,p+n);return true;}
 bool rewind(uint32_t n){at=0;if(corrupt&&!bytes.empty())bytes[0]^=1;return !badLength&&bytes.size()==n;}
 bool read(uint8_t* p,size_t n){if(failRead||at+n>bytes.size())return false;memcpy(p,bytes.data()+at,n);at+=n;return true;}
};
int main(){using P=Backup::Copy::Phase;suite("Streaming SHA-256 and verified copy");DnspHash::Sha256 sha;uint8_t hash[32];DnspHash::shaInit(sha);DnspHash::shaUpdate(sha,(const uint8_t*)"abc",3);DnspHash::shaFinal(sha,hash);char hex[65];for(int i=0;i<32;i++)snprintf(hex+2*i,3,"%02x",hash[i]);ck("SHA256 known vector",!strcmp(hex,"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
 Source s;s.bytes.resize(5231);for(size_t i=0;i<s.bytes.size();i++)s.bytes[i]=i*17;
 for(int scenario=0;scenario<7;scenario++){Dest d;Backup::Copy c;s.fail=scenario==1;d.failWrite=scenario==2;d.failRead=scenario==3;d.corrupt=scenario==4;d.badLength=scenario==5;c.start(s.bytes.size());
  if(scenario==6){c.start(0);ck("empty source rejected",c.phase==P::FAILED);continue;}
  unsigned steps=0;while((c.phase==P::WRITE||c.phase==P::VERIFY)&&steps++<50)c.tick(s,d);
  ck(scenario==0?"read-back completes exact copy":"read/write/corruption failure never completes",c.phase==(scenario==0?P::DONE:P::FAILED));
  if(!scenario)ck("bounded two-pass work",steps==12&&d.bytes==s.bytes);
 }
 return report();}
