#include "integer_scan.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

namespace {
bool space(char c) { return c==' ' || (c>='\t' && c<='\r'); }
}

static int scan(const char* input, const char* format, va_list args) {
    if (!input || !format) return 0;
    const char* begin=input;
    int count=0; bool empty=false;
    while (*format) {
        if (space(*format)) {
            while (space(*format)) ++format;
            while (space(*input)) ++input;
            continue;
        }
        if (*format!='%') {
            if (!*input) { empty=true; break; }
            if (*input++!=*format++) break;
            continue;
        }
        ++format;
        unsigned width=0;
        while (*format>='0' && *format<='9') {
            width=width*10u+unsigned(*format++-'0');
            if (width>63) { return count; }
        }
        const bool wide=*format=='l'; const bool shortNum=*format=='h';
        if(wide || shortNum)++format;
        const char kind=*format;
        if (!kind) break;
        ++format;
        // Reviewed newlib TZ formats also use a bounded ASCII name scanset,
        // %hu and %n (the latter records consumption, not an assignment count).
        if(kind=='n' && !width && !wide && !shortNum) {
            *va_arg(args,int*)=int(input-begin);continue;
        }
        const bool letters=kind=='[' && !strncmp(format,"A-Za-z]",7);
        const bool zoneName=kind=='[' && !strncmp(format,"-+0-9A-Za-z]",12);
        if ((kind!='s' && kind!='d' && kind!='u' && !letters && !zoneName) ||
            ((kind=='s'||kind=='[') && (!width || wide || shortNum)) ||
            (kind!='s' && kind!='[' && width)) break;
        if(kind!='[')while (space(*input)) ++input;
        if (!*input) { empty=true; break; }
        if(kind=='[') {
            format+=letters?7:12;
            auto allowed=[&](char c){return (c>='A'&&c<='Z') || (c>='a'&&c<='z') || (zoneName&&((c>='0'&&c<='9')||c=='-'||c=='+'));};
            if(!allowed(*input))break;
            char* dest=va_arg(args,char*);unsigned n=0;
            while(n<width && allowed(*input))dest[n++]=*input++;
            dest[n]=0;++count;continue;
        }
        if (kind=='s') {
            char* dest=va_arg(args,char*); unsigned n=0;
            while (*input && !space(*input) && n<width) dest[n++]=*input++;
            dest[n]=0;
        } else {
            char* end=nullptr;
            if (kind=='u') {
                const unsigned long v=strtoul(input,&end,10);
                if(end==input)break;
                if(shortNum)*va_arg(args,unsigned short*)=static_cast<unsigned short>(v);
                else if(wide)*va_arg(args,unsigned long*)=v;
                else *va_arg(args,unsigned*)=static_cast<unsigned>(v);
            } else {
                const long v=strtol(input,&end,10);
                if(end==input)break;
                if(shortNum)*va_arg(args,short*)=static_cast<short>(v);
                else if(wide)*va_arg(args,long*)=v;
                else *va_arg(args,int*)=static_cast<int>(v);
            }
            input=end;
        }
        ++count;
    }
    return count ? count : empty ? EOF : 0;
}

int dnspIntegerScan(const char* input, const char* format, ...) {
    va_list args; va_start(args,format);
    const int result=scan(input,format,args);va_end(args);return result;
}
// The build-local, hash-checked SDK object calls this C ABI alias only for
// its literal "%u.%u.%u" version format. Its radio code is otherwise untouched.
extern "C" int dnspIntegerScanC(const char* input,const char* format,...) {
    va_list args;va_start(args,format);
    const int result=scan(input,format,args);va_end(args);return result;
}
