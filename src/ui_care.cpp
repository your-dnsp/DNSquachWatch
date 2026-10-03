#include "ui_care.h"
#include "care.h"
#include "language.h"
#include "theme.h"
#include "field_tools.h"
#include "settings.h"
#include "security.h"
#include "ignore_list.h"
#include "ota_wifi.h"
#include "ota_ble.h"
#include "research.h"
#include "readable_logs.h"
#include <cstdio>
#include <cstring>
namespace CareUI {
static Page current=Page::BACKUP;static uint8_t step=0;static bool dirty=true,confirm=false;static uint32_t drawnAt=0;
static char notice[72]{};
static bool backupPending=false, pendingRendered=false;
static uint8_t sdConfirm=0;
bool working(){return backupPending||Backup::busy()||ReadableLogs::busy();}
void runPending(bool visible,uint32_t now,DetectionEngine& eng){
 if(!visible){backupPending=false;return;}
 if(backupPending&&pendingRendered){backupPending=false;pendingRendered=false;Backup::start(eng.sd().ready(),now,&eng);dirty=true;}
 if(!Backup::busy())ReadableLogs::tick();
}
void drawBackupProgress(TFT_eSPI& t,unsigned percent,const char* phase,uint32_t now){
 if(percent>100)percent=100;const int w=t.width();char line[64];
 snprintf(line,sizeof line,"%s %u%% %c",phase,percent,"|/-\\"[(now/250)%4]);
 Lang::draw(t,line,12,40,w-24,20,Theme::CYAN,false);
 t.drawRect(12,64,w-24,12,Theme::CYAN);
 t.fillRect(14,66,w-28,8,Theme::BG);
 t.fillRect(14,66,(w-28)*percent/100,8,Theme::CYAN);
 if(current==Page::BACKUP && Backup::busy()){
  Lang::draw(t,Backup::progressDetail(),12,80,w-24,16,Theme::WHITE,false);
  snprintf(line,sizeof line,"%lus elapsed | Keep power on",(unsigned long)Backup::elapsedSeconds(now));
  Lang::draw(t,line,12,98,w-24,14,Theme::WHITE,false);
 }else Lang::draw(t,"Keep power on. Please wait.",12,82,w-24,28,Theme::WHITE,false);
}

static uint8_t textPage=0,textPages=1;static int textBottom=88;
void open(Page p){backupPending=false;pendingRendered=false;current=p;step=0;textPage=0;textPages=1;confirm=false;sdConfirm=0;notice[0]=0;dirty=true;}
Page page(){return current;}
bool needsDraw(uint32_t now,int width,int height){static int oldW=0,oldH=0;static uint8_t oldLang=255;
 if(oldW!=width||oldH!=height||oldLang!=Field::config.language){dirty=true;oldW=width;oldH=height;oldLang=Field::config.language;textPage=0;}
 bool live=current==Page::BACKUP||current==Page::STATUS||current==Page::HEALTH||current==Page::REPORT||current==Page::READABLE_LOGS;if(!dirty&&(!live||uint32_t(now-drawnAt)<(working()?250u:1000u)))return false;dirty=false;drawnAt=now;return true;}
static void prose(TFT_eSPI& t,const char* s,int bottom=88){
 textBottom=bottom;int cols=(t.width()-24)/8,lines=(t.height()-58-bottom)/18;if(lines<1)lines=1;
 // These bounded help strings are English ASCII. Wrap into pages rather
 // than silently clipping the final warning off a small display.
 size_t starts[24]{};unsigned pages=1;size_t at=0;int line=0;
 while(s[at]&&pages<24){size_t end=at;int n=0;size_t space=at;
  while(s[end]&&s[end]!='\n'&&n<cols){if(s[end]==' ')space=end;++end;++n;}
  if(s[end]=='\n')++end;else if(s[end]&&space>at)end=space+1;
  if(end==at)++end;at=end;while(s[at]==' ')++at;
  if(++line==lines&&s[at]){starts[pages++]=at;line=0;}
 }
 textPages=pages;if(textPage>=pages)textPage=0;size_t from=starts[textPage],to=textPage+1<pages?starts[textPage+1]:strlen(s);
 char chunk[620];size_t n=to-from;if(n>=sizeof chunk)n=sizeof chunk-1;memcpy(chunk,s+from,n);chunk[n]=0;
 char label[48];snprintf(label,sizeof label,"TEXT %u/%u%s",textPage+1,pages,pages>1?" - TAP TEXT FOR MORE":"");
 t.setTextSize(1);t.setTextColor(Theme::AMBER,Theme::BG);t.setCursor(12,42);t.print(label);
 Lang::draw(t,chunk,12,58,t.width()-24,t.height()-58-bottom,Theme::WHITE,false);
}
static void footer(TFT_eSPI& t,const char* right){int w=t.width(),h=t.height();Lang::button(t,8,h-40,w/2-12,34,current==Page::BACKUP&&working()?"BACK / CANCEL":"BACK");Lang::button(t,w/2+4,h-40,w/2-12,34,right);}
static const char* demos[]={
 "SIMULATED: Possible Flock clue. LOW confidence. A shared manufacturer prefix does not confirm a camera. This sample is never logged or counted.",
 "SIMULATED: Why this matched. Example name + manufacturer clues agree. MEDIUM means stronger evidence, not proof. Confirm visually before reporting.",
 "SIMULATED: Drone / FPV. Remote ID can describe a broadcasting aircraft. An ELRS setup name only suggests equipment; it does not prove flight.",
 "SIMULATED: Meta equipment clue. Bluetooth does not reveal whether glasses are recording. No signal is not an all-clear. This is a practice card."
};
void draw(TFT_eSPI& t,uint32_t now,DetectionEngine& eng){
 int w=t.width(),h=t.height();t.fillRect(0,0,w,h,Theme::BG);Theme::drawTitleBar(t,"DNSP");
 const char* titles[]={"BACKUP & RECOVERY","PRACTICE - SIMULATED","WHY NO MATCH?","GIFT PREPARATION","SESSION REPORT","DEVICE HEALTH","A GIFT FROM DNSP","MICROSD RECOVERY","READABLE LOG EXPORT"};
 Lang::draw(t,titles[(uint8_t)current],22,20,w-44,18,Theme::CYAN,true,true);char b[620];
 if(current==Page::BACKUP){
  snprintf(b,sizeof b,"%s\nSlot %u. %s",confirm?"Restore settings, labels, rules, ignores and operational choices? Historical logs remain readable and are not replayed.":Backup::status(),Backup::slot(),Backup::busy()?"Cancel leaves an incomplete slot.":"Includes firmware, flash-only user state and complete readable history. PIN and Duress data are never included.");if(working()){if(backupPending)pendingRendered=true;textPages=1;drawBackupProgress(t,backupPending?0:Backup::percent(),backupPending?"PREPARING":Backup::phaseLabel(),now);}else prose(t,b,132);
  Lang::button(t,8,h-126,w-16,36,working()?"CANCEL BACKUP":confirm?"CONFIRM RESTORE":"BACK UP TO MICROSD");
  Lang::button(t,8,h-84,w-16,36,working()?"BACKUP IN PROGRESS":confirm?"CANCEL RESTORE":"RESTORE USER STATE");
 }else if(current==Page::DEMO){prose(t,demos[step%4]);snprintf(b,sizeof b,"%u/4 - no effect on history",step+1);Lang::draw(t,b,12,h-76,w-24,26,Theme::AMBER);}
 else if(current==Page::STATUS){
  if(step==0){snprintf(b,sizeof b,"Since boot: BLE adverts %lu; WiFi frames %lu. Types enabled %u. Alert threshold: %s. Ignored devices %u. Prefix quiet: %s; multi-clue only: %s. These counts show reception, not complete coverage.",(unsigned long)advertsSeen(),(unsigned long)wifiFramesSeen(),Settings::enabledTypeCount(),Settings::minConfidenceLabel(),IgnoreList::count(),Field::config.quietPrefix?"yes":"no",Field::config.compositeOnly?"yes":"no");prose(t,b);}
  else if(step==1){snprintf(b,sizeof b,"BLE queue losses %lu; low-memory advert drops %lu; alert overflow %lu. Mesh, scanning and WiFi share radio time. Auto-snooze and mutes may suppress popups while history continues. Disabled types are not logged.",(unsigned long)eng.bleQueueDropped(),(unsigned long)advertsDropped(),(unsigned long)eng.alerts.dropped());prose(t,b);}
  else prose(t,"Only observable 2.4GHz WiFi/Bluetooth clues are supported. Wired, cellular, silent or unfamiliar devices can be invisible. Signal strength is not distance. Check Alerts & Detection and Research coverage. No detection does not mean no camera.");
 }else if(current==Page::GIFT){
  const char* texts[]={"Review before gifting: saved WiFi, PIN and squad credentials, detection history, research files, telemetry config and recovery backups. Nothing is erased by this checklist.","Saved WiFi can contain your passwords. Remove unwanted networks using System > WiFi Networks. A screen PIN does not encrypt flash or the microSD.","Review Security and SquachMesh with the friend. PINs and squad credentials are personal. New installs keep update checks and remote-update requests off until chosen.","Review logs, research files, telemetry config and backups on a computer. Clear visible history alone may leave other files and BlackBox records. Use a fresh card and a deliberate reset before gifting sensitive data.","Choose language, brightness and accessibility. Run the practice cards together. Hardware/endurance and a USB recovery drill are still required before calling this gift-ready.","Arm gift setup for next boot: touch calibration, color check, then a hello from DNSP. Display orientation resets to locked landscape. This does not erase personal data or certify privacy cleanup."};
  prose(t,texts[step%6],132);Lang::button(t,8,h-126,w-16,36,step==1?"REVIEW WIFI":step==2?"REVIEW SECURITY":step==4?"ACCESSIBILITY":"PREVIEW HELLO");
  Lang::button(t,8,h-84,w-16,36,Care::giftArmed()?"GIFT SETUP ARMED - CANCEL":"ARM NEXT-BOOT GIFT SETUP");
 }else if(current==Page::WELCOME)prose(t,"A little radio curiosity, from DNSP. Start with the walkthrough and explore at your own pace. Radio clues are possibilities, not proof. May your loot be silly and your explanations clear!");
 else if(current==Page::REPORT){auto s=Research::stats();snprintf(b,sizeof b,"Last session: %lu; %s. Saved %lu; omitted %lu; errors %lu. A readable, identifier-free report is saved when a session finishes. Source JSONL/CSV contains evidence; raw mode can include private data. %s",(unsigned long)s.session,s.active?"RECORDING":!Research::settled()?"SAVING - KEEP POWER ON":"finished",(unsigned long)s.saved,(unsigned long)s.dropped,(unsigned long)s.errors,notice);prose(t,b);}
 else if(current==Page::HEALTH){auto v=Care::health();snprintf(b,sizeof b,"%s Uptime %lus; loops %lu. Min heap %lu bytes; min contiguous block %lu. Longest loop gap %lums; gaps over 250ms: %lu. %s This is a measurement log, not a hardware pass. Use the endurance checklist.",notice,(unsigned long)(now/1000),(unsigned long)v.loops,(unsigned long)v.minHeap,(unsigned long)v.minBlock,(unsigned long)v.maxGap,(unsigned long)v.over250,Care::bootReady(now)?"30-second boot check reached.":"Boot check pending.");prose(t,b);}
 else if(current==Page::SD_RECOVERY){
  if(sdConfirm){snprintf(b,sizeof b,"FORMAT ERASES THE ENTIRE MICROSD: backups, exports and unrelated files. This cannot be undone. Confirmation step %u of 4.",(unsigned)sdConfirm);prose(t,b,132);Lang::button(t,8,h-126,w-16,36,sdConfirm==1?"STEP 1 - TAP 3":sdConfirm==2?"STEP 2 - TAP 2":sdConfirm==3?"STEP 3 - TAP 1":"FORMAT NOW");Lang::button(t,8,h-84,w-16,36,"CANCEL FORMAT");}
  else {snprintf(b,sizeof b,"%s\nFind & Remount is non-destructive. Test Card creates, reads and removes one tiny file. Format is the last resort and erases the entire card.",eng.sd().recoveryStatus());prose(t,b,174);Lang::button(t,8,h-168,w-16,36,"FIND & REMOUNT");Lang::button(t,8,h-126,w-16,36,"TEST CARD");Lang::button(t,8,h-84,w-16,36,"FORMAT MICROSD...");}
 }
 else if(current==Page::READABLE_LOGS){
  if(ReadableLogs::busy())drawBackupProgress(t,ReadableLogs::percent(),ReadableLogs::phase(),now);
  else {snprintf(b,sizeof b,"%s\nRefresh adds newly stored scan and system events without deleting older readable records. Export & Organize creates a numbered permanent snapshot. Full identifiers are included.",ReadableLogs::status());prose(t,b,132);Lang::button(t,8,h-126,w-16,36,"REFRESH FILES");Lang::button(t,8,h-84,w-16,36,"EXPORT & ORGANIZE");}
 }
 if(current!=Page::SD_RECOVERY&&current!=Page::READABLE_LOGS)footer(t,current==Page::BACKUP?(working()?"PLEASE WAIT":"NEXT SLOT"):(current==Page::REPORT||current==Page::HEALTH)?"EXPORT":current==Page::WELCOME?"WALKTHROUGH":"NEXT");
 else if(current==Page::READABLE_LOGS)Lang::button(t,8,h-40,w-16,34,ReadableLogs::busy()?"CANCEL":"BACK");
 else Lang::button(t,8,h-40,w-16,34,"BACK");

}
SettingsRow tap(int x,int y,int w,int h,uint32_t now,DetectionEngine& eng){
 if(x<8||x>=w-8||y<40)return SettingsRow::NONE;dirty=true;
 if(current==Page::SD_RECOVERY){
  if(y>=h-40)return SettingsRow::BACK;
  if(sdConfirm){
   if(y>=h-84&&y<h-48){sdConfirm=0;return SettingsRow::NONE;}
   if(y>=h-126&&y<h-90){if(sdConfirm<4)sdConfirm++;else{sdConfirm=0;eng.sd().recoveryFormat();}return SettingsRow::NONE;}
  }else{
   if(y>=h-168&&y<h-132)eng.sd().recoveryRemount();
   else if(y>=h-126&&y<h-90)eng.sd().recoveryTest();
   else if(y>=h-84&&y<h-48)sdConfirm=1;
  }
  return SettingsRow::NONE;
 }
 if(current==Page::READABLE_LOGS){
  if(y>=h-40){if(ReadableLogs::busy()){ReadableLogs::cancel();return SettingsRow::NONE;}return SettingsRow::BACK;}
  if(!ReadableLogs::busy()&&y>=h-126&&y<h-90)ReadableLogs::start(ReadableLogs::Mode::REFRESH);
  else if(!ReadableLogs::busy()&&y>=h-84&&y<h-48)ReadableLogs::start(ReadableLogs::Mode::SNAPSHOT);
  return SettingsRow::NONE;
 }
 if(y<h-textBottom&&textPages>1){textPage=(textPage+1)%textPages;return SettingsRow::NONE;}
 if(y>=h-40&&y<h-6){
  if(x<w/2){if(Backup::busy())Backup::cancel();return SettingsRow::BACK;}
  textPage=0;
  if(current==Page::BACKUP&&!working()){confirm=false;Backup::nextSlot();}
  else if(current==Page::WELCOME)return SettingsRow::DNSP_GUIDE;
  else if(current==Page::REPORT){auto s=Research::stats();bool ok=eng.sd().ready()&&!s.active&&Research::settled()&&s.session&&Research::storageReport(s);strcpy(notice,ok?"Saved /dnsp-field-report.txt":"No finished session, card, or export failed.");}
  else if(current==Page::HEALTH){
   auto v=Care::health();char report[1024];snprintf(report,sizeof report,"DNSquachWatch v1.5 device-health report\nUptime seconds: %lu\nLoop count: %lu\nMinimum free heap: %lu\nMinimum largest block: %lu\nMaximum loop gap ms: %lu\nLoop gaps >250ms: %lu\nBLE adverts: %lu\nWiFi frames: %lu\nBLE queue drops: %lu\nAdvert pressure drops: %lu\nAlert overflow: %lu\nBackup verified this boot: %s\nThese measurements do not prove coverage, frame rate, or electrical stability. No identifiers included.\n",
   (unsigned long)(now/1000),(unsigned long)v.loops,(unsigned long)v.minHeap,(unsigned long)v.minBlock,(unsigned long)v.maxGap,(unsigned long)v.over250,(unsigned long)advertsSeen(),(unsigned long)wifiFramesSeen(),(unsigned long)eng.bleQueueDropped(),(unsigned long)advertsDropped(),(unsigned long)eng.alerts.dropped(),Backup::verifiedThisBoot()?"yes":"no");
   strcpy(notice,"Exporting device health...");
   bool ok=Backup::exportHealth(eng.sd().ready(),report);
   if(ok)snprintf(notice,sizeof notice,"Exported to %s",Backup::healthExportPath());
   else strcpy(notice,"Export failed. Check microSD and try again.");
   textPage=0;
  }
  else if(current==Page::DEMO)step=(step+1)%4;else if(current==Page::STATUS)step=(step+1)%3;else if(current==Page::GIFT)step=(step+1)%6;
  return SettingsRow::NONE;
 }
 if(current==Page::BACKUP){
  if(y>=h-126&&y<h-90){if(working()){backupPending=false;Backup::cancel();}else if(confirm){Backup::restore(eng.sd().ready(),&eng);confirm=false;}else {backupPending=true;pendingRendered=false;textPage=0;}}
  if(y>=h-84&&y<h-48&&!working()){confirm=!confirm;textPage=0;}
 }else if(current==Page::GIFT){
  if(y>=h-126&&y<h-90){if(step==1)return SettingsRow::WIFI_NETWORKS;if(step==2)return SettingsRow::SECURITY;if(step==4)return SettingsRow::ACCESSIBILITY;open(Page::WELCOME);}
  if(y>=h-84&&y<h-48)Care::armGift(!Care::giftArmed());
 }
 return SettingsRow::NONE;
}
}
