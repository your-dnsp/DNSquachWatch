#include "ui_labels.h"
#include "theme.h"
#include "privacy.h"
#include "research_submission.h"
#include <cstring>
#include <cstdio>

namespace LabelUI {
namespace {
enum class Stage:uint8_t{CHOICE,TYPE,SUBTAG,T9,CONFIRM,ERROR,SHARE,SHARE_DONE,SHARE_ERROR};
bool on=false;
Stage stage=Stage::CHOICE;
UserLabels::Target target;
UserLabels::Label label;
bool hadLabel=false;
char sharePath[96]{};
uint8_t typePage=0,subPage=0;
uint8_t lastKey=255;
uint32_t lastKeyAt=0;

static const uint8_t TYPES[]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,UserLabels::OTHER_TAG};
static const char* SUBTAGS[]={"NO SUBTAG","CAMERA","SENSOR","GUNSHOT DETECTOR","DOORBELL","GLASSES","TRACKER","DRONE","RADIO","TYPE CUSTOM..."};
constexpr uint8_t TYPE_N=sizeof(TYPES)/sizeof(TYPES[0]);
constexpr uint8_t SUB_N=sizeof(SUBTAGS)/sizeof(SUBTAGS[0]);

void title(TFT_eSPI& t,const char* s){Theme::drawTitleBar(t,"USER DEVICE LABEL");t.setTextFont(1);t.setTextSize(1);t.setTextColor(Theme::CYAN,Theme::BG);t.setCursor(10,18);t.print(s);}
void footer(TFT_eSPI& t,const char* left,const char* right){const int w=t.width(),h=t.height();if(left&&left[0])Theme::drawButton(t,8,h-40,right&&right[0]?w/2-12:w-16,32,left,false);if(right&&right[0])Theme::drawButton(t,w/2+4,h-40,w/2-12,32,right,false);}
void prose(TFT_eSPI& t,const char* s,int y,int maxH){t.setTextSize(1);t.setTextColor(Theme::WHITE,Theme::BG);char lines[12][48];uint8_t n=Theme::wrapText(t,s,t.width()-20,lines,12);for(uint8_t i=0;i<n&&i*11<maxH;i++){t.setCursor(10,y+i*11);t.print(lines[i]);}}
void row(TFT_eSPI& t,int i,const char* text){Theme::drawButton(t,10,42+i*36,t.width()-20,31,text,false);}
void startType(){stage=Stage::TYPE;typePage=0;}
void startSubtag(){stage=Stage::SUBTAG;subPage=label.type==UserLabels::OTHER_TAG?2:0;label.subtag[0]=0;}
void appendT9(uint8_t key,uint32_t now){
    static const char* chars[]={"1","ABC2","DEF3","GHI4","JKL5","MNO6","PQRS7","TUV8","WXYZ9"};
    if(key>=9)return;const char* set=chars[key];size_t n=strlen(label.subtag),m=strlen(set);
    if(lastKey==key&&now-lastKeyAt<900&&n){const char* p=strchr(set,label.subtag[n-1]);label.subtag[n-1]=p?set[(p-set+1)%m]:set[0];}
    else if(n<UserLabels::MAX_SUBTAG){label.subtag[n]=set[0];label.subtag[n+1]=0;}
    lastKey=key;lastKeyAt=now;
}
}

void open(const UserLabels::Target& t){target=t;hadLabel=UserLabels::lookup(t.mac,label);if(!hadLabel){label.type=(uint8_t)t.original;label.subtag[0]=0;}stage=(t.original==DetectionType::UNKNOWN&&!hadLabel)?Stage::TYPE:Stage::CHOICE;typePage=0;subPage=0;lastKey=255;on=true;}
bool active(){return on;}
void close(){on=false;}

void draw(TFT_eSPI& t){
    const int w=t.width(),h=t.height();t.fillRect(0,0,w,h,Theme::BG);t.setTextWrap(false);
    if(stage==Stage::CHOICE){
        title(t,hadLabel?"EDIT USER LABEL":"CONFIRM OR EDIT ASSUMPTION");
        char b[96];snprintf(b,sizeof b,"Detected: %s\nCurrent user label: %s%s%s",detectionTypeName(target.original),hadLabel?UserLabels::typeName(label.type):"not set",hadLabel&&label.subtag[0]?" / ":"",hadLabel?label.subtag:"");prose(t,b,40,55);
        Theme::drawButton(t,10,100,w-20,30,hadLabel?"EXPORT CURRENT AGAIN":"CONFIRM DETECTED TAG",false);
        Theme::drawButton(t,10,136,w-20,30,"EDIT TAG / SUBTAG",false);
        Theme::drawButton(t,10,172,w-20,24,"RESEARCH REPORT...",false);footer(t,"CANCEL","");
    }else if(stage==Stage::SHARE){
        title(t,"UNVERIFIED - REVIEW BEFORE SHARING");
        prose(t,"One device observation, not an OUI-wide identification. Review your subtag for personal details. No location or other logs included.",36,58);
        prose(t,"REDACTED: submit this copy. PRIVATE: retain it; never upload it. MAC suffix XX:XX:XX; names omitted from REDACTED.",98,72);
        
        footer(t,"BACK","EXPORT BOTH");
    }else if(stage==Stage::SHARE_DONE||stage==Stage::SHARE_ERROR){
        title(t,stage==Stage::SHARE_DONE?"REPORT EXPORTED":"REPORT NOT EXPORTED");
        prose(t,stage==Stage::SHARE_DONE?sharePath:"Check microSD mounting and free space in Storage & Recovery, then retry.",40,58);
        prose(t,"Submit only REDACTED. Retain PRIVATE. Paste REDACTED contents into the GitHub form; the checker scans pasted text.",110,65);
        footer(t,"BACK","");
    }else if(stage==Stage::TYPE){
        title(t,"CHOOSE TAG");const uint8_t from=typePage*4;for(uint8_t i=0;i<4&&from+i<TYPE_N;i++)row(t,i,UserLabels::typeName(TYPES[from+i]));
        char more[20];snprintf(more,sizeof more,"MORE %u/%u",typePage+1,(TYPE_N+3)/4);footer(t,"BACK",more);
    }else if(stage==Stage::SUBTAG){
        title(t,label.type==UserLabels::OTHER_TAG?"OTHER REQUIRES A SUBTAG":"OPTIONAL SUBTAG");const uint8_t from=subPage*4;for(uint8_t i=0;i<4&&from+i<SUB_N;i++){if(label.type==UserLabels::OTHER_TAG&&from+i==0)continue;row(t,i,SUBTAGS[from+i]);}
        char more[20];snprintf(more,sizeof more,"MORE %u/%u",subPage+1,(SUB_N+3)/4);footer(t,"BACK",more);
    }else if(stage==Stage::T9){
        title(t,"T9 SUBTAG - UP TO 24 CHARACTERS");t.setTextSize(2);t.setTextColor(Theme::WHITE,Theme::BG);t.setCursor(10,38);t.printf("%s_",label.subtag);
        static const char* keys[]={"1","ABC2","DEF3","DEL","GHI4","JKL5","MNO6","SPACE","PQRS7","TUV8","WXYZ9","DONE"};
        for(int i=0;i<12;i++){int col=i%4,rowIx=i/4;Theme::drawButton(t,6+col*(w-8)/4,68+rowIx*38,(w-16)/4,32,keys[i],false);}footer(t,"BACK","");
    }else if(stage==Stage::CONFIRM){
        title(t,"REVIEW USER OBSERVATION");char mac[24],b[240];snprintf(mac,sizeof mac,"%02X:%02X:%02X:%02X:%02X:%02X",target.mac[0],target.mac[1],target.mac[2],target.mac[3],target.mac[4],target.mac[5]);Privacy::mac(mac,sizeof mac,target.mac);snprintf(b,sizeof b,"MAC: %s\nDetected: %s\nUser label: %s\nSubtag: %s\n\nThis records your observation; it does not prove device identity.",mac,detectionTypeName(target.original),UserLabels::typeName(label.type),label.subtag[0]?label.subtag:"(none)");prose(t,b,40,112);Theme::drawButton(t,10,h-84,w-20,34,"SAVE + EXPORT TO microSD",false);footer(t,"BACK","");
    }else{
        title(t,"EXPORT DID NOT FINISH");prose(t,"The label was not applied because its required microSD record could not be written. Check Storage & Recovery, then retry.",48,90);Theme::drawButton(t,10,h-84,w-20,34,"RETRY EXPORT",false);footer(t,"CANCEL","");
    }
}

Outcome tap(int x,int y,int w,int h,uint32_t now){
    if(!on)return Outcome::NONE;
    if(stage==Stage::CHOICE){
        if(y>=172&&y<196){stage=Stage::SHARE;return Outcome::NONE;}
        if(y>=100&&y<130){if(!hadLabel){label.type=(uint8_t)target.original;label.subtag[0]=0;}stage=Stage::CONFIRM;return Outcome::NONE;}
        if(y>=136&&y<166){startType();return Outcome::NONE;}
        if(y>=h-40&&x<w/2){on=false;return Outcome::CANCELLED;}
    }else if(stage==Stage::SHARE){
        
        if(y>=h-40){if(x<w/2)stage=Stage::CHOICE;else stage=ResearchSubmission::savePair(target,label,now,sharePath,sizeof sharePath)?Stage::SHARE_DONE:Stage::SHARE_ERROR;}
        return Outcome::NONE;
    }else if(stage==Stage::SHARE_DONE||stage==Stage::SHARE_ERROR){
        if(y>=h-40)stage=Stage::SHARE;return Outcome::NONE;
    }else if(stage==Stage::TYPE){
        if(y>=42&&y<186){uint8_t i=(uint8_t)((y-42)/36),at=(uint8_t)(typePage*4+i);if(i<4&&at<TYPE_N){label.type=TYPES[at];startSubtag();}return Outcome::NONE;}
        if(y>=h-40){if(x<w/2){stage=Stage::CHOICE;}else typePage=(uint8_t)((typePage+1)%((TYPE_N+3)/4));return Outcome::NONE;}
    }else if(stage==Stage::SUBTAG){
        if(y>=42&&y<186){uint8_t i=(uint8_t)((y-42)/36),at=(uint8_t)(subPage*4+i);if(i<4&&at<SUB_N){if(at==0&&label.type!=UserLabels::OTHER_TAG){label.subtag[0]=0;stage=Stage::CONFIRM;}else if(at==SUB_N-1){label.subtag[0]=0;lastKey=255;stage=Stage::T9;}else{snprintf(label.subtag,sizeof label.subtag,"%s",SUBTAGS[at]);stage=Stage::CONFIRM;}}return Outcome::NONE;}
        if(y>=h-40){if(x<w/2)stage=Stage::TYPE;else subPage=(uint8_t)((subPage+1)%((SUB_N+3)/4));return Outcome::NONE;}
    }else if(stage==Stage::T9){
        if(y>=68&&y<182){int col=x*4/w;if(col<0)col=0;if(col>3)col=3;int rowIx=(y-68)/38,key=rowIx*4+col;if(key==3){size_t n=strlen(label.subtag);if(n)label.subtag[n-1]=0;lastKey=255;}else if(key==7){size_t n=strlen(label.subtag);if(n<UserLabels::MAX_SUBTAG){label.subtag[n]=' ';label.subtag[n+1]=0;}lastKey=255;}else if(key==11){if(label.subtag[0])stage=Stage::CONFIRM;}else{static const uint8_t map[]={0,1,2,255,3,4,5,255,6,7,8,255};appendT9(map[key],now);}return Outcome::NONE;}
        if(y>=h-40&&x<w/2){stage=Stage::SUBTAG;return Outcome::NONE;}
    }else if(stage==Stage::CONFIRM){
        if(y>=h-84&&y<h-50){if(UserLabels::save(target,label,now)){on=false;return Outcome::SAVED;}stage=Stage::ERROR;return Outcome::NONE;}
        if(y>=h-40&&x<w/2){stage=Stage::SUBTAG;return Outcome::NONE;}
    }else{
        if(y>=h-84&&y<h-50){stage=Stage::CONFIRM;return Outcome::NONE;}
        if(y>=h-40&&x<w/2){on=false;return Outcome::CANCELLED;}
    }
    return Outcome::NONE;
}
} // namespace LabelUI
