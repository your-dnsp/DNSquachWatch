#include "ui_research.h"
#include "research.h"
#include "theme.h"
#include <cstdio>
namespace ResearchUI {
static uint8_t page=0,selection=0,note=0;static Research::Profile profile=Research::Profile::BALANCED;static bool raw=false,confirmRaw=false;
static Research::Record selected; static bool haveSelected=false;
static const char* notes[]={"No note","Control location","Stationary observation","Moving observation","Camera visually nearby"};
void open(){page=0;selection=0;confirmRaw=false;haveSelected=false;}
// Wrap long URLs as well as prose; never silently discard a long word.
static void text(TFT_eSPI& t,const char* msg,int y,int bottom=0) {
    if(!bottom)bottom=t.height()-40;
    const int maxW=t.width()-20;
    while(*msg && y+11<=bottom) {
        while(*msg==' ') ++msg;
        if(!*msg) break;
        char line[48]{};size_t n=0,lastSpace=0;
        while(msg[n] && n<sizeof(line)-1) {
            line[n]=msg[n];line[n+1]=0;
            if(t.textWidth(line)>maxW){line[n]=0;break;}
            if(msg[n]==' ')lastSpace=n;
            ++n;
        }
        if(!n)break;
        if(msg[n]&&lastSpace){n=lastSpace;line[n]=0;}
        t.setCursor(10,y);t.print(line);msg+=n;y+=11;
    }
}

const char* modeLabel(){
    auto s=Research::stats();bool currentRaw=(s.active||!Research::settled())?s.raw:raw;
    return currentRaw?"CURRENT MODE: RAW":"CURRENT MODE: REDACTED";
}
const char* modeAction(){
    if(Research::active())return "MODE LOCKED WHILE RECORDING";
    if(!Research::settled())return "MODE LOCKED WHILE SAVING";
    return confirmRaw?"CONFIRM RAW (PRIVATE DATA)":raw?"SWITCH TO REDACTED":"SWITCH TO RAW...";
}
void sessionLine(char* out,size_t cap,uint32_t now){
    auto s=Research::stats();const uint32_t seconds=(s.active?uint32_t(now-s.start):s.elapsed)/1000;
    const char* state=s.active?"REC":!Research::settled()?"SAVING":s.errors?"ERROR":s.session?"FINISHED":"IDLE";
    if(!s.session&&!s.active&&Research::settled())snprintf(out,cap,"IDLE - NOT RECORDING");
    else snprintf(out,cap,"%s %02lu:%02lu | %lu saved",state,(unsigned long)(seconds/60),(unsigned long)(seconds%60),(unsigned long)s.saved);
}
void draw(TFT_eSPI& t,uint32_t now){
    const int w=t.width(),h=t.height();auto s=Research::stats();t.fillRect(0,0,w,h,Theme::BG);Theme::drawTitleBar(t,"RESEARCH LAB");t.setTextSize(1);t.setTextWrap(false);t.setTextColor(Theme::WHITE,Theme::BG);
    const char* titles[]={"1/7 SESSION","2/7 COVERAGE","3/7 FIELD NOTES","4/7 SIGNATURE PACKS","5/7 DEVICE SUPPORT","6/7 CAMERA RESOURCES","7/7 PUBLIC RECORDS"};t.setCursor(10,20);t.print(titles[page]);char buf[380];
    if(page==0){
        sessionLine(buf,sizeof buf,now);t.setTextColor(s.active?Theme::GREEN:Theme::AMBER,Theme::BG);
        t.setCursor(10,36);t.print(buf);t.setTextColor(Theme::WHITE,Theme::BG);
        t.setCursor(10,50);t.print(modeLabel());
        if(confirmRaw)text(t,"RAW includes MAC addresses and radio payloads. Tap CONFIRM RAW to enable; current mode stays redacted until confirmed.",66,h-124);
        else {snprintf(buf,sizeof buf,"%s. Five minutes / 1 MiB max. JSONL + CSV. %s",Research::status(),(s.active?s.raw:raw)?"RAW includes private identifiers.":"Redacted omits names, payloads and notes.");text(t,buf,66,h-124);}
        Theme::drawButton(t,10,h-120,w-20,24,Research::profileName((s.active||!Research::settled())?s.profile:profile),false);
        Theme::drawButton(t,10,h-92,w-20,24,modeAction(),false);
        Theme::drawButton(t,10,h-64,w-20,24,s.active?"STOP + SAVE":!Research::settled()?"SAVING - PLEASE WAIT":confirmRaw?"CONFIRM MODE ABOVE":"START SESSION",false);
    } else if(page==1){
        unsigned channels=0;for(int i=1;i<=13;i++)if(s.channels&(1u<<i))channels++;
        snprintf(buf,sizeof buf,"%s. Elapsed %lus. Observations %lu; saved %lu; omitted %lu; errors %lu. DEAUTH frames %lu; coherent bursts %lu; multi-target %lu. BLE enabled %lus; WiFi enabled %lus; channels visited %u. %lu KiB written. Counts cover observed channel dwell time, not every transmitted frame. No detection does not mean no camera.",s.active?"RECORDING":!Research::settled()?"SAVING":"STOPPED",(unsigned long)(s.elapsed/1000),(unsigned long)s.observed,(unsigned long)s.saved,(unsigned long)s.dropped,(unsigned long)s.errors,(unsigned long)s.deauthFrames,(unsigned long)s.deauthBursts,(unsigned long)s.deauthMultiTargetBursts,(unsigned long)(s.bleMs/1000),(unsigned long)(s.wifiMs/1000),channels,(unsigned long)(s.bytes/1024));text(t,buf,36);
    } else if(page==2){
        Research::Record r=selected;if(haveSelected){snprintf(buf,sizeof buf,"Record %lu: %s, %ddBm, %lus. A visual sighting nearby does not prove which radio belongs to it. Note: %s",(unsigned long)r.id,detectionTypeName(r.match.type),r.rssi,(unsigned long)(r.at/1000),notes[note]);text(t,buf,36);}else text(t,"No saved observation yet. Start a session with microSD. Notes reference record IDs; radio confidence stays separate. Redacted exports omit note text.",36);
        Theme::drawButton(t,10,h-120,w/2-14,24,"OLDER",false);Theme::drawButton(t,w/2+4,h-120,w/2-14,24,"NOTE",false);
        Theme::drawButton(t,10,h-92,w-20,24,"VISUALLY SEEN NEARBY",false);
        Theme::drawButton(t,10,h-64,w/2-14,24,"SUSPECTED",false);Theme::drawButton(t,w/2+4,h-64,w/2-14,24,"FALSE POS",false);
    } else if(page==3){
        snprintf(buf,sizeof buf,"Catalog %lu. Import /dnsp-signatures.txt from microSD. Data only, max 12 experimental rules / 4096 bytes. Manual import after reboot. One-level rollback. Sources and format: RESEARCH-GUIDE.md. %s",(unsigned long)Research::catalogVersion(),Research::status());text(t,buf,36);
        Theme::drawButton(t,10,h-92,w-20,24,"IMPORT PACK",false);Theme::drawButton(t,10,h-64,w-20,24,"ROLL BACK",false);
    } else if(page==4) text(t,"Axon/TASER: experimental equipment clues. Meta: possible glasses, never recording status. Flock: camera-name and accessory clues. Raven: experimental. L6Q / AutoVu: no model-validated rule yet. Shared vendor prefixes do not confirm ALPR. Wired/cellular cameras may be invisible. Details and citations in RESEARCH-GUIDE.md.",36);
    if(page==5)text(t,"Open on your phone or computer: deflock.org/report and alprradar.com. Visually verify the camera before reporting. Additional map: ringmast4r.github.io/FLOCK (third-party records; may be stale). A map marker is separate from live radio evidence.",36);
    if(page==6)text(t,"Ask your municipality about camera contracts, retention policies, data sharing and oversight. Public-records template toolkit: github.com/rpriven/flock-public-records-toolkit. Check current local requirements before using a template. No request is sent by this device.",36);
    Theme::drawButton(t,10,h-32,w/2-14,24,s.active?"STOP / BACK":"BACK",false);Theme::drawButton(t,w/2+4,h-32,w/2-14,24,"NEXT PAGE",false);
}
bool tap(int x,int y,int w,int h,uint32_t now,bool card,uint32_t id){
    if(x<10||x>w-10)return false;
    if(y>=h-32&&y<h-8){if(x<w/2){Research::stop();return true;}page=(page+1)%7;confirmRaw=false;if(page==2){selection=0;haveSelected=Research::select(0,selected);}return false;}
    if(page==0){
        if(y>=h-120&&y<h-96&&Research::settled())profile=(Research::Profile)(((unsigned)profile+1)%3);
        if(y>=h-92&&y<h-68&&Research::settled()){if(raw){raw=false;confirmRaw=false;haveSelected=false;}else if(confirmRaw){raw=true;confirmRaw=false;haveSelected=false;}else confirmRaw=true;}
        if(y>=h-64&&y<h-40){if(Research::active())Research::stop();else if(!confirmRaw&&Research::settled())Research::start(profile,raw,300000,now,id,card);}
    }else if(page==2){
        if(y>=h-120&&y<h-96){if(x<w/2){Research::Record r;if(Research::recent(selection+1,r))selection++;else selection=0;haveSelected=Research::select(selection,selected);}else note=(note+1)%5;}
        Research::Record r=selected;if(!haveSelected)return false;Research::Verdict v=Research::Verdict::UNREVIEWED;
        if(y>=h-92&&y<h-68)v=Research::Verdict::VISUAL;
        if(y>=h-64&&y<h-40)v=x<w/2?Research::Verdict::SUSPECTED:Research::Verdict::FALSE_POSITIVE;
        if(v!=Research::Verdict::UNREVIEWED)Research::annotate(r.id,v,notes[note],now);
    }else if(page==3&&!Research::active()){
        if(y>=h-92&&y<h-68)Research::storageImport();if(y>=h-64&&y<h-40)Research::rollback();
    }return false;
}
}
