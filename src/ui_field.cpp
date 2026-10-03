#include "ui_field.h"
#include "ui_scratch.h"
#include "location_label.h"
#include "field_tools.h"
#include "drone_watch.h"
#include "research.h"
#include "language.h"
#include "theme.h"
#include "settings.h"
#include "sketchy_rule.h"
#include "blackbox.h"
#include "scan_profile.h"
#include "readable_logs.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#if defined(ARDUINO_ARCH_ESP32)
#include <SD.h>
#include <esp_system.h>
#endif
namespace FieldUI {
static bool dirty = true;
static bool captureConfirm = false;
static bool languageHeld = false, languageHoldFired = false;
static uint32_t languageDownAt = 0;
static int languageDownX = 0, languageDownY = 0;
void cancelInput() { languageHeld = languageHoldFired = false; }
#if !defined(ARDUINO_ARCH_ESP32)
static uint32_t renders = 0;
uint32_t drawCount() {
    return renders;
}
#endif
static Page page = PIT, entryPage = PIT;
static uint8_t selected = 0, slot = 0, help = 0, mutedSlot = 0;
static uint16_t historyIndex=0;
static char status[64]{};
static uint8_t chosenMac[6]{};
static bool haveChosen = false;
static bool lightActive=false;
static uint8_t lightMode=0, morseChar=0;
static char morseText[13]="SOS";
static uint32_t lightAt=0,lastLightTap=0;
static bool timerRunning=false;
static uint32_t timerAt=0,timerSaved=0;
static int32_t counter=0;
static uint32_t randomValue=0;
static bool randomReady=false;
static bool randomAnimating=false;
static uint32_t randomStarted=0;
static constexpr uint32_t RANDOM_ANIMATION_MS=1000;
static uint8_t readerOffset=0,readerCount=0,readerPick=0;
static bool readerOpen=false;
static bool readerRefresh=false;
static uint32_t radioPrevWifi=0,radioPrevBle=0,radioSampleAt=0;
static uint32_t radioWifiRate=0,radioBleRate=0;
static uint32_t random32(){
#if defined(ARDUINO_ARCH_ESP32)
    return esp_random();
#else
    return (uint32_t)random(0,0x7fffffff);
#endif
}
bool keepsAwake(){return page==SCREEN_LIGHT&&lightActive;}

static const char* lightNames[]={"WHITE LIGHT","AMBER LIGHT","RAINBOW LIGHT","CAUTION PULSE","SOS MORSE","CUSTOM MORSE"};
static const char* morseFor(char c){
    static const char* letters[]={".-","-...","-.-.","-..",".","..-.","--.","....","..",".---","-.-",".-..","--","-.","---",".--.","--.-",".-.","...","-","..-","...-",".--","-..-","-.--","--.."};
    static const char* digits[]={"-----",".----","..---","...--","....-",".....","-....","--...","---..","----."};
    if(c>='A'&&c<='Z')return letters[c-'A'];if(c>='0'&&c<='9')return digits[c-'0'];return "";
}
static bool morseOn(uint32_t elapsed){
    const char* s=lightMode==4?"SOS":morseText;const uint32_t unit=240;uint32_t total=0;
    for(size_t i=0;s[i];i++){if(s[i]==' '){total+=7;continue;}const char* m=morseFor(s[i]);for(size_t j=0;m[j];j++){total+=m[j]=='-'?3:1;if(m[j+1])total++;}total+=3;}
    if(!total)return false;uint32_t u=(elapsed/unit)%total;total=0;
    for(size_t i=0;s[i];i++){
        if(s[i]==' '){total+=7;continue;}const char* m=morseFor(s[i]);
        for(size_t j=0;m[j];j++){uint32_t on=m[j]=='-'?3:1;if(u>=total&&u<total+on)return true;total+=on;if(m[j+1])total+=1;}
        total+=3;
    }
    return false;
}
static void loadReaderList(){
    UiScratch::storage.reader=UiScratch::Reader{};
    readerCount=0;readerOpen=false;UiScratch::storage.reader.text[0]=0;
#if defined(ARDUINO_ARCH_ESP32)
    const char* known[]={"ALL-ALERTS.txt","SCAN-HISTORY.txt","SYSTEM-HISTORY.txt","SKETCHY-ENVIRONMENT.txt","EXPORT-SUMMARY.txt"};
    for(const char* n:known){char p[96];snprintf(p,sizeof p,"/DNSP Readable Logs/Current/%s",n);if(SD.exists(p)&&readerCount<16)snprintf(UiScratch::storage.reader.names[readerCount++],96,"%s",p);}
    File root=SD.open("/");if(!root)return;File f;
    while((f=root.openNextFile())&&readerCount<16){const char* n=f.name();size_t z=strlen(n);if(!f.isDirectory()&&z>4&&!strcasecmp(n+z-4,".txt")){snprintf(UiScratch::storage.reader.names[readerCount++],96,"%s",n);}f.close();}root.close();
#endif
}
static bool openReader(uint8_t i){
    UiScratch::storage.reader.text[0]=0;if(i>=readerCount)return false;
#if defined(ARDUINO_ARCH_ESP32)
    File f=SD.open(UiScratch::storage.reader.names[i],FILE_READ);if(!f)return false;f.seek(readerOffset);size_t n=f.read((uint8_t*)UiScratch::storage.reader.text,sizeof(UiScratch::storage.reader.text)-1);f.close();UiScratch::storage.reader.text[n]=0;readerOpen=true;return true;
#else
    return false;
#endif
}
void open() {
    cancelInput();
    dirty = true;
    page = entryPage = PIT;
    selected = slot = help = 0;
    status[0] = 0;
    Lang::resetTaps();
    lightActive=false;readerOpen=false;readerOffset=0;randomAnimating=false;
}
void openPage(Page p) { open(); page=entryPage=p;if(p==POCKET_READER)loadReaderList(); }
void openHelp() { openPage(HELP); }
void openLanguage() { openPage(LANGUAGE); }
void openAccessibility() { openPage(ACCESS); }
void openRules() { openPage(RULES); }
void openRuleHistory() { openPage(RULE_HISTORY); }
void openAlertHistory() { openPage(ALERT_HISTORY_SCAN); historyIndex=0; }
void openScanProfiles() { openPage(SCAN_PROFILES); }
bool needsDraw(uint32_t now, int width, int height) {
    static Field::Config previous;
    static uint32_t drawnAt = 0;
    static int oldW = 0, oldH = 0;
    bool changed =
        memcmp(&previous, &Field::config, sizeof previous) != 0 || oldW != width || oldH != height;
    if(readerRefresh){ReadableLogs::tick();if(!ReadableLogs::busy()){readerRefresh=false;loadReaderList();snprintf(status,sizeof status,"%.58s",ReadableLogs::status());}}
    bool live = page == DRONES || page == TELEMETRY || page == SENSORS || page == DISCOVER || page == DRONE_DIAG || page == DRONE_CAPTURE || page == DRONE_TOOLS || page==SCREEN_LIGHT || page==TIMER_COUNTER || page==RADIO_ACTIVITY || page==SCAN_COMPARE || readerRefresh || (page==RANDOMIZER&&randomAnimating);
    if (!dirty && !changed && (!live || now - drawnAt < 250))
        return false;
    dirty = false;
    previous = Field::config;
    oldW = width;
    oldH = height;
    drawnAt = now;
    return true;
}
uint8_t currentPage() {
    return page;
}
static int rowY(int h, int i) {
    return 40 + i * ((h - 88) / 4);
}
static int rowH(int h) {
    return (h - 88) / 4 - 3;
}
static void line(TFT_eSPI &t, const char *text, int y, bool translate = true) {
    Lang::draw(t, text, 10, y, t.width() - 20, 18, Theme::WHITE, translate);
}
static void paragraph(TFT_eSPI &t, const char *text, int y, int h) {
    Lang::draw(t, text, 10, y, t.width() - 20, h, Theme::WHITE);
}
static void macText(char *b, size_t n, const uint8_t *m) {
    snprintf(b, n, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
}
static void footer(TFT_eSPI &t, const char *right) {
    int w = t.width(), h = t.height();
    bool left = Field::config.left;
    Lang::button(t, left ? w / 2 + 2 : 8, h - 40, w / 2 - 10, 34, "BACK");
    Lang::button(t, left ? 8 : w / 2 + 2, h - 40, w / 2 - 10, 34, right);
}
static void row(TFT_eSPI &t, int i, const char *label, bool on = false) {
    Lang::button(t, 8, rowY(t.height(), i), t.width() - 16, rowH(t.height()), label, on);
}
static const char *helpText[] = {
    "Welcome. This device observes radio clues, not people or intent.",
    "Use Why this matched to inspect evidence. A possible match is not confirmation.",
    "No detection does not mean no camera.",
    "FPV equipment clues do not prove a drone is flying.",
    "Choose channels together before powering video transmitters. This board does not measure 5.8 "
    "GHz.",
    "Use microSD for exports. Raw research files can contain device identifiers.",
    "Verify a camera visually before reporting it. Open deflock.org/report on your phone.",
    "An upstream update replaces DNSP firmware. Reinstall a DNSP image from your-dnsp."};
void draw(TFT_eSPI &t, uint32_t now, const DetectionEngine &eng) {
#if !defined(ARDUINO_ARCH_ESP32)
    renders++;
#endif
    int w = t.width(), h = t.height();
    t.fillRect(0, 0, w, h, Theme::BG);
    t.setTextWrap(false);
    Theme::drawTitleBar(t, (page==RULES||(page>=RULE_EVIDENCE&&page<=RULE_ABOUT))?"RULES":page==ALERT_HISTORY_SCAN||page==ALERT_HISTORY_SYSTEM||page==SCAN_PROFILES||page==SCAN_CUSTOM||page==SCAN_COMPARE?"ALERTS & DETECTION":page==ACCESS||page==LANGUAGE?"ACCESSIBILITY & LANGUAGE":page==TELEMETRY||page==SENSORS||page==DISCOVER||page==RADIO_ACTIVITY?"RESEARCH & DATA":page==HELP||page>=SCREEN_LIGHT?"DNSP'S TOOLS":"FPV & DRONES");
    const char *titles[] = {"", "", "FPV PIT BOARD", "DRONE READINGS",
                            "OWN TELEMETRY", "MY SENSORS",  "ALERT RULES",   "ACCESSIBILITY",
                            "LANGUAGE",      "HELP",        "DISCOVER", "DRONE TOOLS", "RID DIAGNOSTICS", "RID CAPTURE", "RECEIVER LIMITS",
                            "SCREEN LIGHT", "RANDOMIZER", "TIMER & COUNTER", "POCKET READER", "RADIO ACTIVITY",
                            "EVIDENCE & MUTES", "RECENT RULE ALERTS", "ABOUT RULES",
                            "SCAN HISTORY", "DEVICE & SYSTEM", "DETECTION PROFILE", "CUSTOM SCANNING", "PROFILE COMPARISON"};
    Lang::draw(t, titles[page], 32, 16, w - 64, 18, Theme::CYAN, true, true);
    char b[240]{};
    if(page==SCREEN_LIGHT&&lightActive){
        uint16_t c=Theme::WHITE;uint32_t el=now-lightAt;
        if(lightMode==1)c=Theme::AMBER;
        else if(lightMode==2){
            for(int i=0;i<12;i++){uint8_t phase=(uint8_t)(i*21+now/18);uint8_t r=phase<85?255-phase*3:phase<170?0:(phase-170)*3;uint8_t g=phase<85?phase*3:phase<170?255-(phase-85)*3:0;uint8_t bl=phase<85?0:phase<170?(phase-85)*3:255-(phase-170)*3;t.fillRect(i*w/12,0,w/12+1,h,t.color565(r,g,bl));}
        }else if(lightMode==3){uint16_t q=(uint16_t)((el%1800)<900?(el%900)*256/900:(900-el%900)*256/900);c=Theme::blend(Theme::BG,Theme::AMBER,q);}
        else if(lightMode>=4)c=morseOn(el)?Theme::WHITE:Theme::BG;
        if(lightMode!=2)t.fillRect(0,0,w,h,c);
        t.fillRect(0,h-28,w,28,Theme::BG);t.setTextFont(1);t.setTextSize(1);t.setTextColor(Theme::WHITE,Theme::BG);t.setCursor(8,h-20);t.print("DOUBLE-TAP ANYWHERE TO STOP");
        return;
    }
    if (page == PIT) {
        for (int i = 0; i < 4; i++) {
            bool warn = false;
            for (int j = 0; j < 4; j++)
                warn |= Field::conflict(i, j);
            snprintf(b, sizeof b, "P%d  %c%u  %u MHz  %s", i + 1, "ABEFR"[Field::config.bands[i]],
                     Field::config.channels[i],
                     Field::frequency(Field::config.bands[i], Field::config.channels[i]),
                     warn ? "!" : "");
            row(t, i, b);
        }
    } else if (page == DRONES) {
        const auto &a = Field::aircraft(selected);
        snprintf(b, sizeof b, "%u/4  %s", selected + 1,
                 Field::associatedAircraft(selected,now)>=0 ? "WiFi + BLE (same claimed ID)" : a.wifi ? "WiFi Remote ID" : "BLE Remote ID");
        line(t, b, 42, false);
        if (!a.used)
            paragraph(t, "NO DATA", 64, 54);
        else {
            snprintf(b, sizeof b, "ID: %.20s", a.info.haveBasic ? a.info.serial : "--");
            line(t, b, 64, false);
            if (a.info.haveLoc) {
                snprintf(b, sizeof b, "%.5f, %.5f", (double)a.info.lat, (double)a.info.lon);
                line(t, b, 84, false);
                snprintf(b, sizeof b, "Altitude %.0fm; age %lus", (double)a.info.altM,
                         (unsigned long)((now - a.info.locAt) / 1000));
                line(t, b, 104, false);
            }
            paragraph(t, Field::identityConflict(selected,now) ? "Conflicting identity / positions" : RemoteId::qualityText(a.info, now), 126, h - 216);
        }
        Lang::button(t,8,h-84,w-16,36,"DRONE TOOLS");
    } else if (page == DRONE_TOOLS) {
        row(t,0,DroneWatch::focused()?"SEARCH: FOCUSED (tap: normal)":"SEARCH: NORMAL (tap: focus)");
        row(t,1,"RECEPTION DIAGNOSTICS");row(t,2,"CAPTURE TO microSD");row(t,3,"LIMITS / MORE INFO");
    } else if (page == DRONE_DIAG || page == DRONE_CAPTURE) {
        const auto s=DroneWatch::stats();
        t.setTextFont(1);t.setTextSize(1);t.setTextColor(Theme::WHITE,Theme::BG);
        auto small=[&](int y,const char *text){t.setCursor(8,y);t.print(text);};
        if(page==DRONE_DIAG){
            snprintf(b,sizeof b,"%s | channel %u",DroneWatch::focused()?(DroneWatch::wifiPhase(now)?"FOCUS: WiFi":"FOCUS: BLE"):"NORMAL",s.channel);small(42,b);
            snprintf(b,sizeof b,"Received WiFi %lu / BLE %lu",(unsigned long)s.wifi,(unsigned long)s.ble);small(58,b);
            snprintf(b,sizeof b,"Decoded %lu | clues %lu",(unsigned long)s.decoded,(unsigned long)s.clues);small(74,b);
            snprintf(b,sizeof b,"Unsupported %lu | malformed %lu",(unsigned long)s.unsupported,(unsigned long)s.malformed);small(90,b);
            snprintf(b,sizeof b,"Channel errors %lu | queue drops %lu",(unsigned long)s.channelErrors,(unsigned long)Field::dropped());small(106,b);
            small(124,DroneWatch::verdictText(s.last));small(144,"Counts are packets, not aircraft.");
            small(160,"No detection is not clearance.");
        } else {
            const char *label=s.capture==DroneWatch::Capture::STARTING?"PREPARING...":s.capture==DroneWatch::Capture::RECORDING?"RECORDING":s.capture==DroneWatch::Capture::SAVING?"SAVING...":s.capture==DroneWatch::Capture::ERROR?"ERROR: microSD write failed":s.capture==DroneWatch::Capture::DONE?"FINISHED":"IDLE - NOT RECORDING";
            small(42,label);
            if(s.capture==DroneWatch::Capture::STARTING)DroneWatch::captureReady();
            uint32_t seconds=(s.capture==DroneWatch::Capture::RECORDING?now-s.started:s.elapsed)/1000;
            snprintf(b,sizeof b,"%02lu:%02lu | %lu saved | %lu dropped",(unsigned long)(seconds/60),(unsigned long)(seconds%60),(unsigned long)s.saved,(unsigned long)s.dropped);small(58,b);
            small(78,"RAW: device IDs and positions.");small(94,"60s / ~512 candidate packets max.");
            small(110,"/dnsp-rid-capture.jsonl");
            snprintf(b,sizeof b,"Truncated: %lu (1024-byte cap)",(unsigned long)s.truncated);small(126,b);
            Lang::button(t,8,h-84,w-16,36,!DroneWatch::settled()?"STOP AND SAVE":captureConfirm?"CONFIRM RAW CAPTURE":"START RAW CAPTURE...");
        }
    } else if(page==DRONE_LIMITS){
        paragraph(t,"2.4 GHz WiFi and legacy BLE only. Focus alternates 15s WiFi / 5s BLE; other alerts have gaps. IDs are broadcast claims. Use a compatible external receiver for 5 GHz or BLE 5. Capture stops on exit or lock.",42,h-90);
    } else if (page == TELEMETRY) {
        const auto &d = Field::telemetry();
        if (!Field::telemetryActive())
            paragraph(t, Field::telemetryStatus(), 42, h - 132);
        else {
            snprintf(b, sizeof b, "Heartbeat: %s",
                     d.heartbeat && (now - d.heartbeatAt) < 5000 ? "received" : "STALE / none");
            line(t, b, 42, false);
            if (d.battery) {
                snprintf(b, sizeof b, "%.2fV %d%% %s", d.millivolts / 1000., d.remaining,
                         now - d.batteryAt > 5000 ? "STALE" : "");
                line(t, b, 62, false);
            }
            if (d.position) {
                snprintf(b, sizeof b, "%.5f, %.5f", (double)d.lat, (double)d.lon);
                line(t, b, 82, false);
                snprintf(b, sizeof b, "Position age %lus",
                         (unsigned long)((now - d.positionAt) / 1000));
                line(t, b, 102, false);
            } else
                line(t, "NO DATA", 82);
            if (d.link) {
                snprintf(b, sizeof b, "Link %u / %u %s", d.rssi, d.remoteRssi,
                         now - d.linkAt > 5000 ? "STALE" : "");
                line(t, b, 122, false);
            }
        }
        Lang::button(t, 8, h - 84, w - 16, 36, Field::telemetryActive() ? "DISCONNECT" : "CONNECT");
    } else if (page == SENSORS) {
        const auto &s = Field::sensor(slot);
        snprintf(b, sizeof b, "Sensor slot %u/4", slot + 1);
        line(t, b, 42, false);
        if (Field::config.sensorOn[slot]) {
            macText(b, sizeof b, Field::config.sensors[slot]);
            line(t, b, 62, false);
        }
        if (s.used && Field::config.sensorOn[slot] &&
            !memcmp(s.mac, Field::config.sensors[slot], 6)) {
            snprintf(b, sizeof b, "%s  age %lus", now - s.at > 60000 ? "STALE" : "",
                     (unsigned long)((now - s.at) / 1000));
            line(t, b, 82, false);
            if (s.fields & 1) {
                snprintf(b, sizeof b, "Temp %.2f C %s", s.temperature / 100.,
                         s.trend ? (s.temperature > s.previous   ? "+"
                                    : s.temperature < s.previous ? "-"
                                                                 : "=")
                                 : "");
                line(t, b, 102, false);
            }
            if (s.fields & 2) {
                snprintf(b, sizeof b, "Humidity %.2f %%", s.humidity / 100.);
                line(t, b, 122, false);
            }
            if (s.fields & 4) {
                snprintf(b, sizeof b, "Battery %u %%", s.battery);
                line(t, b, 142, false);
            }
        } else
            line(t, "NO DATA", 86);
        Lang::button(t, 8, h - 80, w / 2 - 10, 34, "DISCOVER");
        Lang::button(t, w / 2 + 2, h - 80, w / 2 - 10, 34, "FORGET SENSOR");
    } else if (page == DISCOVER) {
        const auto &s = Field::discovery(selected);
        snprintf(b, sizeof b, "%u/4 -> slot %u", selected + 1, slot + 1);
        line(t, b, 42, false);
        if (s.used) {
            macText(b, sizeof b, s.mac);
            line(t, b, 66, false);
            snprintf(b, sizeof b, "Age %lus", (unsigned long)((now - s.at) / 1000));
            line(t, b, 88, false);
            Lang::button(t, 8, h - 84, w - 16, 36, "PIN SENSOR");
        } else
            line(t, "NO DATA", 66);
    } else if (page == RULES) {
        char recent[40];snprintf(recent,sizeof recent,"RECENT ALERTS: %u",(unsigned)SketchyRule::count());
        row(t,0,"SKETCHY ENVIRONMENT",SketchyRule::enabled());
        row(t,1,recent);
        row(t,2,"EVIDENCE & MUTES  >");
        row(t,3,"ABOUT RULES  >");
    } else if (page == RULE_EVIDENCE) {
        row(t, 0, "QUIET PREFIXES", Field::config.quietPrefix);
        row(t, 1, "COMPOSITE ONLY", Field::config.compositeOnly);
        row(t, 2, "MUTE SELECTED");
        row(t, 3, "CLEAR MUTES");
    } else if(page==RULE_HISTORY){
        SketchyRule::Incident in{};if(SketchyRule::recent(selected,in)){char a[24],d[24];macText(a,sizeof a,in.alpr.mac);macText(d,sizeof d,in.deauth.mac);snprintf(b,sizeof b,"%u/%u  %s + DEAUTH\nGap: %lus\nALPR: %s\nDeauth: %s\nmicroSD: %s",selected+1,SketchyRule::count(),detectionTypeName(in.alpr.type),(unsigned long)in.gapSeconds,a,d,in.sdExported?"saved":"pending");paragraph(t,b,44,h-94);}else paragraph(t,"No Sketchy Environment rule alerts have been recorded.",48,h-100);
    } else if(page==RULE_ABOUT){
        paragraph(t,"Rules combine separate observations into a secondary alert. Sketchy Environment watches for Flock, Axon or another ALPR clue and a deauthentication burst within 90 seconds. It is a caution signal, not proof the events are related. Custom rule creation is planned, but is not available in this version.",42,h-92);
    } else if(page==ALERT_HISTORY_SCAN){
        BlackBox::DetRecord r{};uint16_t count=BlackBox::detectionsKept();
        if(BlackBox::readDetections(historyIndex,1,&r)){char m[24];macText(m,sizeof m,r.mac);snprintf(b,sizeof b,"%u/%u  %s%s\nBoot %u +%lus\nMAC %s\nRSSI %d  CH %u  hits %u\n%s | %s\nLocation: %s",(unsigned)(historyIndex+1),(unsigned)count,detectionTypeName((DetectionType)r.type),(r.flags&BlackBox::DET_AGAIN)?" - REAPPEARED":"",(unsigned)r.boot,(unsigned long)r.upSec,m,(int)r.rssi,(unsigned)r.channel,(unsigned)r.hits,r.conf==(uint8_t)Confidence::HIGH_CONF?"HIGH":r.conf==(uint8_t)Confidence::MED_CONF?"MEDIUM":"LOW",r.name[0]?r.name:r.vendor,LocationLabel::text(BlackBox::locationKey(r)));paragraph(t,b,42,h-90);}else paragraph(t,"No stored scan events. Snoozed and popup-suppressed detections appear here once observed and stored.",44,h-96);
    } else if(page==ALERT_HISTORY_SYSTEM){
        BlackBox::BootRecord r{};uint16_t count=BlackBox::bootsKept();
        if(BlackBox::readBoots(historyIndex,1,&r)){snprintf(b,sizeof b,"%u/%u  BOOT %u\nReason: %s\nFirmware: %s\nPrior uptime: %lus\nHeap: %lu free / %lu block\nScreen %u  Task: %s",(unsigned)(historyIndex+1),(unsigned)count,(unsigned)r.boot,BlackBox::reasonName(r.reason),r.version,(unsigned long)r.upSec,(unsigned long)r.heapFree,(unsigned long)r.heapBlock,(unsigned)r.screen,r.task[0]?r.task:"--");paragraph(t,b,42,h-90);}else paragraph(t,"No stored device or system records are available.",44,h-96);
    } else if(page==SCAN_PROFILES){
        for(uint8_t i=0;i<5;i++){char q[34];snprintf(q,sizeof q,"%s%s",ScanProfile::name((ScanProfile::Profile)i),ScanProfile::current()==i?"  [ON]":"");Lang::button(t,8,34+i*25,w-16,23,q,ScanProfile::current()==i);}
        paragraph(t,ScanProfile::description(ScanProfile::current()),160,h-202);
    } else if(page==SCAN_CUSTOM){
        snprintf(b,sizeof b,"WIFI SWEEP: %.1f SEC",ScanProfile::customWifiMs()/1000.0);row(t,0,b);
        snprintf(b,sizeof b,"BLUETOOTH SHARE: %u%%",(unsigned)ScanProfile::customBleShare());row(t,1,b);
        {int x0=12,x1=w-12,y0=rowY(h,0)+rowH(h)-8;int fill=(x1-x0-4)*(ScanProfile::customWifiMs()-2000)/6000;t.drawRect(x0,y0,x1-x0,6,Theme::CYAN);t.fillRect(x0+2,y0+2,fill,2,Theme::AMBER);}
        {int x0=12,x1=w-12,y0=rowY(h,1)+rowH(h)-8;int fill=(x1-x0-4)*(ScanProfile::customBleShare()-40)/45;t.drawRect(x0,y0,x1-x0,6,Theme::CYAN);t.fillRect(x0+2,y0+2,fill,2,Theme::AMBER);}
        snprintf(b,sizeof b,"MAXIMUM: %s",ScanProfile::maximumMinutes()==255?"UNTIL CHANGED":ScanProfile::maximumMinutes()==60?"60 MIN":ScanProfile::maximumMinutes()==30?"30 MIN":"10 MIN");row(t,2,b);
        paragraph(t,"Tap left/right to change. WiFi and Bluetooth share one radio.",rowY(h,3),h-rowY(h,3)-46);
    } else if(page==SCAN_COMPARE){
        if(ScanProfile::comparing()){snprintf(b,sizeof b,"Testing %s\n%u%% complete\nAbout three minutes total. Keep the device in one place.",ScanProfile::name((ScanProfile::Profile)ScanProfile::comparisonProfile()),ScanProfile::comparisonPercent(now));paragraph(t,b,46,h-130);}
        else if(ScanProfile::comparisonDone()){const auto&r=ScanProfile::result(selected%4);snprintf(b,sizeof b,"%u/4 %s\nWiFi frames: %lu\nBLE adverts: %lu\nChannel sweeps: %lu\nStored events: %lu\nDropped events: %lu\nMin memory block: %lu",(unsigned)(selected%4+1),ScanProfile::name((ScanProfile::Profile)(selected%4)),(unsigned long)r.wifiFrames,(unsigned long)r.bleAdverts,(unsigned long)r.sweeps,(unsigned long)r.detections,(unsigned long)r.drops,(unsigned long)r.minBlock);paragraph(t,b,42,h-92);}
        else paragraph(t,"Runs Stationary, Balanced, Fast Sweep and Maximum for 45 seconds each. Results describe this place and moment, not a permanent ranking.",44,h-132);
        if(!ScanProfile::comparing())Lang::button(t,8,h-84,w-16,36,ScanProfile::comparisonDone()?"RUN AGAIN":"START 3-MINUTE COMPARISON");
    } else if (page == ACCESS) {
        row(t, 0, "HIGH CONTRAST", Field::config.contrast);
        row(t, 1, "REDUCED MOTION", Field::config.reduced);
        row(t, 2, "LEFT HANDED", Field::config.left);
        row(t, 3, "LARGE CONTROLS", Field::config.large);
    } else if (page == LANGUAGE) {
        Lang::button(t, 8, 54, w - 16, 40, Lang::name(Field::config.language));
        paragraph(t, "Language preview. Translations need human review.", 108, h - 156);
    } else if (page == SCREEN_LIGHT) {
        Lang::button(t,8,42,w-16,34,lightNames[lightMode]);
        paragraph(t,"The screen becomes the light. Double-tap anywhere to stop.",48+34,w>=400?40:54);
        if(lightMode==5){snprintf(b,sizeof b,"MESSAGE: %s_",morseText);line(t,b,126,false);Lang::button(t,8,148,w/3-7,32,"CHAR -");Lang::button(t,w/3+2,148,w/3-4,32,"ADD / NEXT");Lang::button(t,2*w/3+2,148,w/3-10,32,"DELETE");char c[20];snprintf(c,sizeof c,"NEXT: %c"," ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"[morseChar%37]);line(t,c,184,false);}
        Lang::button(t,8,h-84,w-16,36,"START LIGHT");
    } else if (page == RANDOMIZER) {
        uint32_t elapsed=randomAnimating?now-randomStarted:RANDOM_ANIMATION_MS;
        if(randomAnimating&&elapsed>=RANDOM_ANIMATION_MS){randomAnimating=false;randomReady=true;elapsed=RANDOM_ANIMATION_MS;}
        if(!randomReady&&!randomAnimating)paragraph(t,"Coin Flip makes a two-way choice. Dowsing Rod points in a random direction; it is not a compass.",42,h-140);
        else if(selected==0){
            if(randomAnimating){
                const float phase=(float)elapsed*3.14159265f/125.f;
                const int rx=3+(int)(27.f*fabsf(cosf(phase)));
                t.fillEllipse(w/2,104,rx,30,Theme::AMBER);
                Lang::draw(t,"FLIPPING...",10,138,w-20,24,Theme::CYAN,false,true);
            } else {
                snprintf(b,sizeof b,"%s",(randomValue&1)?"HEADS":"TAILS");
                Lang::draw(t,b,10,76,w-20,60,Theme::CYAN,false,true);
            }
        } else {
            float degrees=(float)(randomValue%360);
            if(randomAnimating){const float remaining=1.f-(float)elapsed/(float)RANDOM_ANIMATION_MS;degrees+=1080.f*remaining*remaining;}
            const float a=degrees*3.14159265f/180.f;int cx=w/2,cy=104,len=(h<280?54:78);int ex=cx+(int)(cosf(a)*len),ey=cy+(int)(sinf(a)*len);
            t.drawLine(cx,cy,ex,ey,Theme::CYAN);t.drawLine(cx+1,cy,ex+1,ey,Theme::CYAN);t.fillCircle(ex,ey,5,Theme::AMBER);
            line(t,randomAnimating?"SPINNING...":"Random direction - not a compass",h-108,false);
        }
        Lang::button(t,8,h-84,w/2-12,36,"COIN FLIP");Lang::button(t,w/2+4,h-84,w/2-12,36,"DOWSING ROD");
    } else if (page == TIMER_COUNTER) {
        const int timerButtonsY=80;
        const int counterLabelY=122;
        const int counterButtonsY=h-84;
        uint32_t ms=timerSaved+(timerRunning?now-timerAt:0);snprintf(b,sizeof b,"%02lu:%02lu.%01lu",(unsigned long)(ms/60000),(unsigned long)((ms/1000)%60),(unsigned long)((ms/100)%10));Lang::draw(t,b,8,45,w-16,42,Theme::CYAN,false,true);
        Lang::button(t,8,timerButtonsY,w/2-12,32,timerRunning?"PAUSE":"START");Lang::button(t,w/2+4,timerButtonsY,w/2-12,32,"RESET TIMER");
        snprintf(b,sizeof b,"COUNTER: %ld",(long)counter);Lang::draw(t,b,8,counterLabelY,w-16,28,Theme::WHITE,false,true);Lang::button(t,8,counterButtonsY,w/3-7,32,"-1");Lang::button(t,w/3+2,counterButtonsY,w/3-4,32,"RESET");Lang::button(t,2*w/3+2,counterButtonsY,w/3-10,32,"+1");
    } else if (page == POCKET_READER) {
        if(readerRefresh){snprintf(b,sizeof b,"%s %u%%",ReadableLogs::phase(),ReadableLogs::percent());line(t,b,46,false);t.drawRect(12,70,w-24,12,Theme::CYAN);t.fillRect(14,72,(w-28)*ReadableLogs::percent()/100,8,Theme::CYAN);paragraph(t,"Refreshing readable files. Keep power on.",92,h-150);}
        else if(readerOpen){paragraph(t,UiScratch::storage.reader.text,40,h-86);line(t,"First 1 KiB; full identifiers may appear",h-54,false);}
        else if(!readerCount)paragraph(t,"No root-level .txt files found, or microSD is unavailable. Put short reference files in the card root.",42,h-96);
        else {for(uint8_t i=0;i<readerCount&&i<6;i++){snprintf(b,sizeof b,"%c %.32s",i==readerPick?'>':' ',UiScratch::storage.reader.names[i]);line(t,b,42+i*22,false);}}
        if(!readerRefresh&&!readerOpen){Lang::button(t,8,h-126,w-16,36,"REFRESH FILES");if(readerCount)Lang::button(t,8,h-84,w-16,36,"OPEN SELECTED");}
    } else if (page == RADIO_ACTIVITY) {
        uint32_t wf=wifiFramesSeen(),ba=advertsSeen();if(!radioSampleAt){radioPrevWifi=wf;radioPrevBle=ba;radioSampleAt=now;}else if(now-radioSampleAt>=1000){uint32_t dt=now-radioSampleAt;radioWifiRate=(wf-radioPrevWifi)*1000/dt;radioBleRate=(ba-radioPrevBle)*1000/dt;radioPrevWifi=wf;radioPrevBle=ba;radioSampleAt=now;}
        snprintf(b,sizeof b,"Wi-Fi %lu frames/s   BLE %lu adverts/s",(unsigned long)radioWifiRate,(unsigned long)radioBleRate);line(t,b,40,false);
        const int top=66,bh=h-138,bw=(w-18)/13;for(int ch=1;ch<=13;ch++){int v=eng.channelActivity(ch),bar=v*bh/100;t.fillRect(4+(ch-1)*bw,top+bh-bar,bw-2,bar,Theme::CYAN);t.setTextFont(1);t.setTextSize(1);t.setTextColor(Theme::WHITE,Theme::BG);t.setCursor(5+(ch-1)*bw,top+bh+3);t.printf("%d",ch);}
        line(t,"Decoded 2.4 GHz Wi-Fi/BLE activity; not a general RF spectrum analyzer.",h-58,false);
    } else if (page == HELP) {
        snprintf(b, sizeof b, "%u/8", help + 1);
        line(t, b, 42, false);
        paragraph(t, helpText[help], 68, h - 116);
    }
    const char *right = page == RULE_HISTORY ? "NEXT ALERT" : page==ALERT_HISTORY_SCAN?"DEVICE / SYSTEM":page==ALERT_HISTORY_SYSTEM?"SCAN HISTORY":page==SCAN_PROFILES?"COMPARE":page==SCAN_CUSTOM?"PROFILES":page==SCAN_COMPARE?"NEXT RESULT":page==RULE_EVIDENCE||page==RULE_ABOUT ? "RULES" : page == DRONE_DIAG || page == DRONE_CAPTURE || page == DRONE_LIMITS ? "TOOLS" : page == DRONE_TOOLS ? "READINGS" : page == PIT         ? "EXPORT TO SD"
                        : page == SENSORS   ? "NEXT"
                        : page == TELEMETRY ? "HELP"
                        : page == LANGUAGE  ? "HELP"
                        : page == POCKET_READER && readerOpen ? "REFRESH FILES"
                        : page == POCKET_READER && !readerOpen ? "NEXT FILE"
                                            : "NEXT";
    footer(t, right);
    if (status[0]) {
        char shortStatus[64];
        unsigned maxChars = (w - 16) / 6;
        snprintf(shortStatus, sizeof shortStatus, "%.*s", (int)maxChars, status);
        t.setTextFont(1);
        t.setTextSize(1);
        t.setTextColor(Theme::AMBER, Theme::BG);
        t.setCursor(8, h - 49);
        t.print(shortStatus);
    }
    (void)eng;
}
// Only the language selector defers its ordinary action until release.
// Leaving its bounds or moving more than a finger's jitter cancels the hold.
bool input(int x, int y, int w, int h, bool down, bool justDown, uint32_t now, const DetectionEngine &eng) {
    const bool inside = x >= 8 && x < w - 8 && y >= 54 && y < 94;
    if (page != LANGUAGE) { cancelInput(); return justDown && tap(x,y,w,h,now,eng); }
    if (justDown) {
        cancelInput();
        if (inside) {
            languageHeld = true; languageDownAt = now;
            languageDownX = x; languageDownY = y;
        } else return tap(x,y,w,h,now,eng);
    }
    if (!languageHeld) return false;
    if (down) {
        if (!inside || x-languageDownX > 16 || languageDownX-x > 16 ||
            y-languageDownY > 16 || languageDownY-y > 16) { cancelInput(); return false; }
        if (!languageHoldFired && now-languageDownAt >= 3000) {
            languageHoldFired = true;
            if (!Field::config.hebrew) { Field::config.hebrew = true; Field::save(); }
            strcpy(status, "Additional language available"); dirty = true;
        }
    } else {
        if (!languageHoldFired && now-languageDownAt <= 600) { Lang::next(); status[0]=0; dirty=true; }
        cancelInput();
    }
    return false;
}
bool tap(int x, int y, int w, int h, uint32_t now, const DetectionEngine &eng) {
    if (x < 0 || x >= w || y < 0 || y >= h)
        return false;
    dirty = true;
    status[0] = 0;
    if(page==SCREEN_LIGHT&&lightActive){if(now-lastLightTap<=550){lightActive=false;lastLightTap=0;}else lastLightTap=now;return false;}
    if(page==SCREEN_LIGHT){
        if(y>=42&&y<76){lightMode=(lightMode+1)%6;return false;}
        if(lightMode==5&&y>=148&&y<180){size_t n=strlen(morseText);if(x<w/3){morseChar=(morseChar+36)%37;}else if(x<2*w/3){if(n<12){char c=" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"[morseChar%37];morseText[n]=c;morseText[n+1]=0;}morseChar=(morseChar+1)%37;}else if(n)morseText[n-1]=0;return false;}
        if(y>=h-84&&y<h-48){if(lightMode!=5||morseText[0]){lightActive=true;lightAt=now;lastLightTap=0;}else strcpy(status,"Enter at least one character");return false;}
    }
    if(page==RANDOMIZER&&y>=h-84&&y<h-48){if(!randomAnimating){selected=x<w/2?0:1;randomValue=random32();randomReady=false;randomAnimating=true;randomStarted=now;}return false;}
    if(page==TIMER_COUNTER){
        if(y>=80&&y<112){if(x<w/2){if(timerRunning){timerSaved+=now-timerAt;timerRunning=false;}else{timerAt=now;timerRunning=true;}}else{timerRunning=false;timerSaved=0;}return false;}
        if(y>=h-84&&y<h-52){if(x<w/3)counter--;else if(x<2*w/3)counter=0;else counter++;return false;}
    }
    if(page==SCAN_PROFILES&&y>=34&&y<34+5*25){uint8_t p=(uint8_t)((y-34)/25);ScanProfile::select((ScanProfile::Profile)p,now);if(p==ScanProfile::CUSTOM)page=SCAN_CUSTOM;return false;}
    if(page==SCAN_CUSTOM){
        if(y>=rowY(h,0)&&y<rowY(h,0)+rowH(h)){int px=x<12?12:x>w-12?w-12:x;ScanProfile::setCustomWifi((uint16_t)(2000+(uint32_t)(px-12)*6000/(w-24)));return false;}
        if(y>=rowY(h,1)&&y<rowY(h,1)+rowH(h)){int px=x<12?12:x>w-12?w-12:x;ScanProfile::setCustomBle((uint8_t)(40+(uint32_t)(px-12)*45/(w-24)));return false;}
        if(y>=rowY(h,2)&&y<rowY(h,2)+rowH(h)){ScanProfile::cycleMaximumMinutes();return false;}
    }
    if(page==SCAN_COMPARE&&!ScanProfile::comparing()&&y>=h-84&&y<h-48){selected=0;ScanProfile::startComparison(now,wifiFramesSeen(),advertsSeen(),eng.channelSweeps(),eng.storedEvents(),eng.bleQueueDropped());return false;}
    if(page==POCKET_READER&&!readerOpen){
        if(y>=42&&y<42+22*readerCount){readerPick=(uint8_t)((y-42)/22);return false;}
        if(y>=h-126&&y<h-90){readerRefresh=ReadableLogs::start(ReadableLogs::Mode::REFRESH);return false;}
        if(y>=h-84&&y<h-48){openReader(readerPick);return false;}
    }
    if (y >= h - 40 && y < h - 6) {
        bool back = (x < w / 2) != bool(Field::config.left);
        if (back) {
            if(page==POCKET_READER&&readerRefresh){ReadableLogs::cancel();readerRefresh=false;return false;}
            if(page==SCAN_COMPARE&&ScanProfile::comparing())ScanProfile::stopComparison();
            if(page==SCAN_CUSTOM||page==SCAN_COMPARE){page=SCAN_PROFILES;return false;}
            if(page==POCKET_READER&&readerOpen){readerOpen=false;readerOffset=0;return false;}
            if(page==RULE_HISTORY&&entryPage==RULE_HISTORY)return true;
            if(page==RULE_EVIDENCE||page==RULE_HISTORY||page==RULE_ABOUT){page=RULES;selected=0;return false;}
            if (page==entryPage) { DroneWatch::stopCapture(); captureConfirm=false; Field::telemetryStop(); Lang::resetTaps(); return true; }
            if(page>=DRONE_TOOLS&&page<=DRONE_LIMITS){DroneWatch::stopCapture();captureConfirm=false;page=page==DRONE_TOOLS?DRONES:DRONE_TOOLS;return false;}
            Field::telemetryStop();
            Lang::resetTaps();
            return true;
        }
        if(page>=DRONE_TOOLS&&page<=DRONE_LIMITS){DroneWatch::stopCapture();captureConfirm=false;page=page==DRONE_TOOLS?DRONES:DRONE_TOOLS;}
        else if(page==POCKET_READER){if(readerOpen){readerRefresh=ReadableLogs::start(ReadableLogs::Mode::REFRESH);}else if(readerCount)readerPick=(readerPick+1)%readerCount;}
        else if (page == PIT)
            strcpy(status,
                   Field::exportPit() ? "Saved /dnsp-fpv-pit.csv" : "microSD export failed");
        else if (page == SENSORS)
            slot = (slot + 1) % 4;
        else if (page == DRONES || page == DISCOVER) {
            for(int tries=0;tries<4;tries++){
                selected=(selected+1)%4;int other=Field::associatedAircraft(selected,now);
                if(page!=DRONES || other<0 || other>selected || Field::identityConflict(selected,now))break;
            }
        }
        else if (page == TELEMETRY) {
            Field::telemetryStop();
            page = HELP;
            help = 3;
        } else if (page == LANGUAGE) {
            Lang::resetTaps();
            page = HELP;
            help = 0;
        } else if (page == HELP)
            help = (help + 1) % 8;
        else if (page == RULE_EVIDENCE) {
            selected = eng.logCount() ? (selected + 1) % eng.logCount() : 0;
            const Detection *d = eng.logAt(selected);
            haveChosen = d != nullptr;
            if (d) {
                memcpy(chosenMac, d->mac, 6);
                macText(status, sizeof status, d->mac);
            }
        } else if(page==RULE_HISTORY&&SketchyRule::count())selected=(selected+1)%SketchyRule::count();
        else if(page==ALERT_HISTORY_SCAN){page=ALERT_HISTORY_SYSTEM;historyIndex=0;}
        else if(page==ALERT_HISTORY_SYSTEM){page=ALERT_HISTORY_SCAN;historyIndex=0;}
        else if(page==SCAN_PROFILES){page=SCAN_COMPARE;selected=0;}
        else if(page==SCAN_CUSTOM){page=SCAN_PROFILES;}
        else if(page==SCAN_COMPARE&&ScanProfile::comparisonDone())selected=(selected+1)%4;
        else if(page==RULE_EVIDENCE||page==RULE_ABOUT){page=RULES;selected=0;}
        return false;
    }
    if((page==ALERT_HISTORY_SCAN||page==ALERT_HISTORY_SYSTEM)&&y>=42&&y<h-42){uint16_t n=page==ALERT_HISTORY_SCAN?BlackBox::detectionsKept():BlackBox::bootsKept();if(n)historyIndex=(historyIndex+1)%n;return false;}
    if(page==DRONES && y>=h-84 && y<h-48){page=DRONE_TOOLS;return false;}
    if(page==DRONE_CAPTURE && y>=h-84 && y<h-48){
        if(!DroneWatch::settled()){DroneWatch::stopCapture();captureConfirm=false;}
        else if(!captureConfirm)captureConfirm=true;
        else {DroneWatch::startCapture(now);captureConfirm=false;}
        return false;
    }
    if (page == LANGUAGE && y >= 54 && y < 94) {
        Lang::next();
        return false;
    }
    if (page == TELEMETRY && y >= h - 84 && y < h - 48) {
        if (Field::telemetryActive())
            Field::telemetryStop();
        else
            Field::telemetryStart();
        return false;
    }
    if (page == SENSORS && y >= h - 80 && y < h - 46) {
        if (x < w / 2) {
            page = DISCOVER;
            selected = 0;
        } else {
            Field::config.sensorOn[slot] = false;
            Field::save();
        }
        return false;
    }
    if (page == DISCOVER && y >= h - 84 && y < h - 48) {
        const auto &s = Field::discovery(selected);
        if (s.used && now - s.at < 60000) {
            memcpy(Field::config.sensors[slot], s.mac, 6);
            Field::config.sensorOn[slot] = true;
            Field::save();
            page = SENSORS;
        }
        return false;
    }
    int r = -1;
    for (int i = 0; i < 4; i++)
        if (y >= rowY(h, i) && y < rowY(h, i) + rowH(h))
            r = i;
    if (r < 0)
        return false;
    if(page==DRONE_TOOLS){
        if(r==0){DroneWatch::setFocused(!DroneWatch::focused(),now);strcpy(status,"Other alerts have listening gaps");}
        if(r==1)page=DRONE_DIAG;
        if(r==2){page=DRONE_CAPTURE;captureConfirm=false;}
        if(r==3)page=DRONE_LIMITS;
    } else if (page == PIT) {
        if (x < w / 3)
            Field::config.bands[r] = (Field::config.bands[r] + 1) % 5;
        else
            Field::config.channels[r] = (Field::config.channels[r] + 1) % 9;
        Field::save();
    } else if (page == ACCESS) {
        uint8_t *options[] = {&Field::config.contrast, &Field::config.reduced, &Field::config.left,
                              &Field::config.large};
        *options[r] = !*options[r];
        Field::save();
        Theme::applyPalette(Settings::paletteIndex());
    } else if(page==RULES){
        if(r==0)SketchyRule::toggle();
        else if(r==1){selected=0;page=RULE_HISTORY;}
        else if(r==2){selected=0;page=RULE_EVIDENCE;}
        else page=RULE_ABOUT;
    } else if (page == RULE_EVIDENCE) {
        if (r == 0)
            Field::config.quietPrefix = !Field::config.quietPrefix;
        if (r == 1)
            Field::config.compositeOnly = !Field::config.compositeOnly;
        if (r == 2) {
            if (haveChosen) {
                memcpy(Field::config.muted[mutedSlot], chosenMac, 6);
                Field::config.muteOn[mutedSlot] = true;
                mutedSlot = (mutedSlot + 1) % 4;
                strcpy(status, "Selected device silenced; still logged");
            } else
                strcpy(status, "Tap NEXT to choose a logged device");
        }
        if (r == 3)
            for (auto &m : Field::config.muteOn)
                m = false;
        Field::save();
    }
    return false;
}
} // namespace FieldUI
