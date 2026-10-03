#include "sd_retry.h"
#include <cassert>
#include <cstdio>
int main(){SdRetry::State s;assert(s.due(0));assert(!s.retire(false,true,0));assert(!s.due(499)&&s.due(500));assert(!s.retire(false,true,500));assert(s.retire(false,true,1000)&&s.attempts==0);assert(s.retire(true,true,1001));assert(s.retire(false,false,1002));assert(!s.retire(false,true,UINT32_MAX-200));assert(!s.due(100)&&s.due(300));assert(s.retire(true,true,300)&&s.attempts==0);puts("SD retry timing, failure limit, unmounted card and rollover PASS");}
