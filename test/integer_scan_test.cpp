#include <initializer_list>
#include "integer_scan.h"
#include "test_util.h"
#include <cstdio>
#include <cstring>
#include <climits>
extern "C" int dnspIntegerScanC(const char*,const char*,...);
int main(){
 suite("Bounded scanner equivalence for reviewed integer/string formats");
 for(const char* s:{"1 2 3","  +1\t-2\n3tail","1 2",""," ","x","1 x 3","-1 0 2147483647"}){
  int a=-9,b=-9,c=-9,x=-9,y=-9,z=-9;
  const int n=sscanf(s,"%d %d %d",&a,&b,&c),m=DNSP_INTEGER_SCAN(s,"%d %d %d",&x,&y,&z);
  ck("signed fields, EOF and partial assignment match scanf",n==m&&a==x&&b==y&&c==z);
 }
 for(const char* s:{"4294967295 0","-1 +12","0 5tail","5","not-a-number",""}){
  unsigned a=7,b=7,x=7,y=7;int n=sscanf(s,"%u %u",&a,&b),m=DNSP_INTEGER_SCAN(s,"%u %u",&x,&y);
  ck("unsigned console settings match scanf",n==m&&a==x&&b==y);
 }
 for(const char* s:{"1.2.3","+1.02.003 extra","1.2"," 1 .2.3","x",""}){
  unsigned a=9,b=9,c=9,x=9,y=9,z=9;
  int n=sscanf(s,"%u.%u.%u",&a,&b,&c),m=dnspIntegerScanC(s,"%u.%u.%u",&x,&y,&z);
  ck("pinned SDK version query retains conversion semantics",n==m&&a==x&&b==y&&c==z);
 }
 for(const char* s:{"id-123 4294967295","abcdefghijklmnopqrstuvwxyz123456789 4","no-number","","id\n12"}){
  char a[32]{},b[32]{};unsigned long x=7,y=7;
  int n=sscanf(s,"%31s %lu",a,&x),m=DNSP_INTEGER_SCAN(s,"%31s %lu",b,&y);
  ck("pending journal ID is bounded and unchanged",n==m&&!strcmp(a,b)&&x==y);
 }
 const char* fmt="DNSP_CHECKPOINT_V2\ngen=%lu\ndet=%lu\nboot=%lu\nrule=%lu\ncrc=%lu\n";
 for(unsigned i=0;i<160;i++){
  char buf[160];snprintf(buf,sizeof buf,fmt,1ul,4294967295ul,2ul,3ul,1234567890ul);
  if(i<strlen(buf))buf[i]=0;
  unsigned long a[5]{},b[5]{};
  int n=sscanf(buf,fmt,a,a+1,a+2,a+3,a+4),m=DNSP_INTEGER_SCAN(buf,fmt,b,b+1,b+2,b+3,b+4);
  ck("checkpoint truncation preserves partial/complete parsing",n==m&&!memcmp(a,b,sizeof a));
 }
 for(const char* fmt:{"det=%lu\nboot=%lu\nrule=%lu","DNSP_SD_COPY_V1\noffset=%lu\ndest=%lu\ncrc=%lu\n"}){
  char buf[128];snprintf(buf,sizeof buf,fmt,12345ul,0ul,4294967295ul);
  unsigned long a[3]{},b[3]{};
  ck("legacy and SD-copy journal fields remain compatible",sscanf(buf,fmt,a,a+1,a+2)==DNSP_INTEGER_SCAN(buf,fmt,b,b+1,b+2)&&!memcmp(a,b,sizeof a));
 }
 suite("Pinned newlib timezone formats: names, offsets, DST and consumed bytes");
 for(const char* fmt:{"%10[A-Za-z]%n","%10[-+0-9A-Za-z]%n"})
 for(const char* in:{"UTC","EST5EDT","+03","-0930","ABCDEFGHIJK"," UTC","_bad",""}){
   char a[11]{},b[11]{};int n=-1,m=-1;
   int x=sscanf(in,fmt,a,&n),y=dnspIntegerScanC(in,fmt,b,&m);
   ck("bounded TZ names match newlib-supported scanf semantics",x==y&&!strcmp(a,b)&&n==m);
 }
 for(const char* fmt:{"%hu%n:%hu%n:%hu%n","M%hu%n.%hu%n.%hu%n","/%hu%n:%hu%n:%hu%n"})
 for(const char* in:{"5","05:30","-1:2:3","12:34:56","M3.2.0","/2:0:0","x",""}){
   unsigned short a[3]{},b[3]{};int n[3]={-1,-1,-1},m[3]={-1,-1,-1};
   int x=sscanf(in,fmt,a,n,a+1,n+1,a+2,n+2),y=dnspIntegerScanC(in,fmt,b,m,b+1,m+1,b+2,m+2);
   ck("TZ offsets, DST rules and partial reads match scanf",x==y&&!memcmp(a,b,sizeof a)&&!memcmp(n,m,sizeof n));
 }
 char safe[4]="ok";
 ck("unbounded string formats are rejected",DNSP_INTEGER_SCAN("longlong","%s",safe)==0&&!strcmp(safe,"ok"));
 return report();
}
