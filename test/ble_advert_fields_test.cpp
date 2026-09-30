#include "test_util.h"
#include "ble_advert_fields.h"
#include <cstring>
int main(){
 suite("Borrowed BLE advertisement fields");
 const uint8_t bytes[]={3,8,'A','B',4,255,1,2,3,3,255,4,5,4,9,'X','Y','Z'};
 auto a=findAdvertField(bytes,sizeof bytes,9);
 ck("short name matches existing getter precedence",a.size()==2 && a.data()==bytes+2);
 auto b=findAdvertField(bytes,sizeof bytes,255,1);
 ck("second manufacturer block",b.size()==2 && b[0]==4 && b[1]==5);
 ck("missing field empty",findAdvertField(bytes,sizeof bytes,22).size()==0);
 ck("index beyond count empty",findAdvertField(bytes,sizeof bytes,255,2).size()==0);
 ck("zero length input safe",findAdvertField(nullptr,0,9).size()==0);
 const uint8_t truncated[]={255,9,'x'};
 ck("truncated field rejected",findAdvertField(truncated,sizeof truncated,9).size()==0);
 const uint8_t padding[]={0,0,2,9,'Z'};
 ck("padding skips safely",findAdvertField(padding,sizeof padding,9).size()==1);
 const uint8_t empty[]={1,9};
 ck("empty named field safe",findAdvertField(empty,sizeof empty,9).size()==0);
 // Exercise every length byte against bounded short buffers under sanitizers.
 uint8_t fuzz[32]={};
 for(unsigned n=0;n<=sizeof fuzz;++n)for(unsigned len=0;len<256;++len){
  fuzz[0]=len;fuzz[1]=255;
  auto f=findAdvertField(fuzz,n,255);
  if(f.size())ck("returned slice remains inside packet",f.data()>=fuzz && f.data()+f.size()<=fuzz+n);
 }
 return report();
}
