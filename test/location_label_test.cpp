#include "location_label.h"
#include "test_util.h"
#include <cstring>
#include <string>
#include <cstdlib>
#include <unistd.h>
using namespace LocationLabel;
int main(){char dir[]="/tmp/dnsp-location-test-XXXXXX";ck("isolated persistent store",mkdtemp(dir)!=nullptr);setenv("SQUACHSIM_NVS",dir,1);begin();
 ck("default explicitly unset",!strcmp(current(),"no-label-set"));
 ck("manual label",set("Driving"));uint32_t driving=currentKey();
 wifi("My authenticated network");ck("unknown connected network keeps manual label",currentKey()==driving&&!recalled());
 ck("set while connected",set("Home"));uint32_t home=currentKey();
 wifi("");ck("manual survives disconnect",currentKey()==home&&!recalled());
 begin();ck("reboot does not carry session label",currentKey()==0);wifi("My authenticated network");ck("authenticated reconnect recalls label",currentKey()==home&&recalled());
 wifi("");ck("recalled label clears on disconnect",currentKey()==0);
 ck("history label preserved",!strcmp(text(home),"Home")&&!strcmp(text(driving),"Driving"));
 wifi("My authenticated network");ck("clear forgets binding",clear());wifi("");wifi("My authenticated network");ck("clear prevents return",currentKey()==0);
 ck("24 characters allowed",set("123456789012345678901234"));ck("25 characters rejected",!set("1234567890123456789012345"));
 ck("empty rejected",!set(""));ck("whitespace rejected",!set("   "));ck("CSV injection rejected",!set("=HYPERLINK"));ck("delimiter rejected",!set("Home,Work"));ck("newline rejected",!set("Home\nWork"));ck("quote rejected",!set("Home\"Work"));
 Snapshot s;capture(s);ck("snapshot validates",validate(s));Snapshot bad=s;bad.labels[0].text[0]^=1;ck("corrupt snapshot rejected",!validate(bad)&&!restore(bad));
 ck("retry restore succeeds",restore(s)&&restore(s));ck("unknown key is honest",!strcmp(text(0xfffffe),"location-unavailable"));
 for(unsigned i=0;i<80;i++){char label[25];snprintf(label,sizeof label,"Spot %u",i);set(label);}Snapshot full;capture(full);unsigned count=0;for(auto&e:full.labels)if(e.key)count++;ck("dictionary bounded",count==MAX_LABELS);ck("full rejects new label",!set("Another new spot"));ck("full allows known label",set("Home"));ck("no eviction of historical label",!strcmp(text(driving),"Driving"));
 ck("restore preflight leaves full dictionary",restore(s)&&!strcmp(text(driving),"Driving"));
 wipe();begin();ck("wipe removes labels and mappings",!currentKey()&&!strcmp(text(home),"location-unavailable"));return report();}
