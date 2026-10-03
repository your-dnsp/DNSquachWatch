#include "installation_guide.h"
#include "test_util.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <SD.h>

static bool collect(void* context,const uint8_t* data,size_t size){auto*out=(std::vector<uint8_t>*)context;out->insert(out->end(),data,data+size);return true;}
int main(){
 suite("Card installation guide");TestSD::root="../microSD-content";
 std::vector<uint8_t> decoded;ck("decode succeeds",InstallationGuide::write(collect,&decoded));
 ck("reported size",decoded.size()==InstallationGuide::size());
 FILE*f=fopen("../DNSQUACHWATCH INSTALLATION.txt","rb");ck("source guide opens",f!=nullptr);std::vector<uint8_t> expected;if(f){uint8_t b[256];for(size_t n;(n=fread(b,1,sizeof b,f));)expected.insert(expected.end(),b,b+n);fclose(f);}
 ck("decoded output is byte-for-byte exact",decoded==expected);
 TestSD::root="out/missing-card";decoded.clear();ck("missing content refuses backup guide",!InstallationGuide::write(collect,&decoded)&&decoded.empty());
 return report();
}
