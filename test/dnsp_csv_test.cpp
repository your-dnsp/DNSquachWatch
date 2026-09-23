#include <initializer_list>
#include "test_util.h"
#include "csv_text.h"
#include <cstring>
int main() {
    char out[24];
    suite("Untrusted radio names in exported CSV");
    for(const char* s : {"=1+1", "+CMD", "-CMD", "@SUM(1)", "   =1+1"}) {
        safeCsvText(out,sizeof out,s,strlen(s));
        const char* p=out;while(*p==' ')++p;
        ck("formula-leading character neutralized",*p=='?');
    }
    const char bad[]="a,b\r\n\t\"c";
    safeCsvText(out,sizeof out,bad,sizeof bad);
    ck("no row or column injection",!strchr(out,',') && !strchr(out,'\r') && !strchr(out,'\n') && !strchr(out,'"') && !strchr(out,'\t'));
    char name[20];memset(name,'A',sizeof name);
    safeCsvText(out,sizeof out,name,sizeof name);
    ck("bounded input needs no NUL",strlen(out)==20);
    struct { char pre;char buf[4];char post; } small{'L',{},'R'};
    safeCsvText(small.buf,4,name,sizeof name);
    ck("small output terminates with canaries intact",small.pre=='L' && small.post=='R' && strlen(small.buf)==3);
    safeCsvText(nullptr,0,name,sizeof name);
    safeCsvText(out,sizeof out,nullptr,100);
    ck("empty input stays empty",out[0]==0);
    bool safe=true;
    for(unsigned byte=1;byte<256;++byte) {
        char input[32];memset(input,byte,sizeof input);
        safeCsvText(out,sizeof out,input,sizeof input);
        safe &= out[23]==0;
        for(unsigned i=0;out[i];++i) safe &= out[i]>=32 && out[i]<=126 && out[i]!=',' && out[i]!='"';
    }
    ck("all byte values remain a bounded printable cell",safe);
    return report();
}
