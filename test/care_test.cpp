#include "test_util.h"
#include "care.h"
#include "field_tools.h"
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>
int main(){
 mkdir("out",0755);mkdir("out/carenvs",0755);setenv("SQUACHSIM_NVS","out/carenvs",1);
 remove("out/carenvs/settings.nvs");remove("out/carenvs/dnsp-care.nvs");remove("out/carenvs/dnsp-field.nvs");
 suite("Public preferences validate before restore");Care::Snapshot a;Care::capture(a);char text[Care::SETTINGS_CAP];
 ck("captured defaults valid",Care::valid(a));ck("encode",Care::encode(a,text,sizeof text));Care::Snapshot b;
 ck("roundtrip",Care::decode(text,strlen(text),b)&&!memcmp(&a,&b,sizeof a));
 bool rejected=true;for(size_t n=0;n<strlen(text);n++)rejected&=!Care::decode(text,n,b);ck("every truncation rejected",rejected);
 std::string corrupt=text;corrupt[20]^=1;ck("corrupt file rejected",!Care::decode(corrupt.data(),corrupt.size(),b));
 corrupt=std::string(text)+"trailing";ck("trailing/unknown settings rejected",!Care::decode(corrupt.data(),corrupt.size(),b));
 ck("no sensitive fields",!strstr(text,"password")&&!strstr(text,"meshtx")&&!strstr(text,"rmtUpd")&&!strstr(text,"pin"));
 a.values[10]=0;ck("blackout brightness rejected",!Care::apply(a));Care::capture(a);a.values[11]=31;ck("invalid alert duration rejected",!Care::valid(a));Care::capture(a);
 a.values[11]=45;a.language=6;a.hebrew=0;ck("hidden language requires reveal state",!Care::valid(a));a.hebrew=1;
 ck("valid restore applies",Care::apply(a));Care::capture(b);ck("public settings survive restore",b.values[11]==45&&b.language==6&&b.hebrew==1);
 suite("Favorites, next-boot gift and health");Care::begin();auto f=Care::favorite(0);Care::cycleFavorite(0);Care::begin();ck("favorite persists",Care::favorite(0)!=f);ck("invalid slot safe",Care::favorite(4)==SettingsRow::NONE);
 Care::armGift(true);ck("arming is not immediate",Care::giftArmed()&&!Care::giftDue());Care::begin();ck("next boot sees hello",Care::giftDue());Care::consumeGift();Care::begin();ck("hello consumed once",!Care::giftDue());
 for(unsigned i=0;i<100;i++)Care::noteLoop(i*400,40000-i,16000-i);auto h=Care::health();ck("health minima and stall counts",h.minHeap==39901&&h.minBlock==15901&&h.maxGap==400&&h.over250==99);ck("responsive boot gate",Care::bootReady(40000));
 return report();
}
