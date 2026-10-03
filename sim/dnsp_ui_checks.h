// Opt-in desktop integration checks against the real firmware setup()/loop().
#pragma once
#include "location_label.h"
#include "../test/test_util.h"
#include "settings.h"
#include "ui_device.h"
#include "crash_reports.h"
#include <filesystem>
#include "alert_snooze.h"
#include "ui_boot.h"
#include "ui_colorcheck.h"
#include "squachy.h"
#include "ui_sysprops.h"
#include "research.h"
#include "ui_research.h"
#include "ui_field.h"
#include "ui_breakout.h"
#include "ui_care.h"
#include "care.h"
#include "field_tools.h"
#include "drone_watch.h"
#include "scan_profile.h"
#include "language.h"
#include "theme.h"
#include "ui_settings.h"
#include "security.h"
#include "ui_update.h"
#include "png_writer.h"
extern uint32_t alertStart;
extern uint32_t lastTouch;
extern uint32_t transitionStart;
extern DetectionType lastAlertType;

static void testStep(uint32_t ms=33) { SimClock::nowMs+=ms; loop(); }
static void testTap(int x,int y) {
    testStep(300);
    screenToRaw(x,y,SimTouch::rawX,SimTouch::rawY);SimTouch::down=true;testStep();
    SimTouch::down=false;testStep();
}
static void testShot(const char* name) {
    testStep(250); // capture settled pixels after the transition animation
    for(int settle=0;settle<4;settle++)testStep(33);
    const char* dir=getenv("DNSP_TEST_SHOTS");if(!dir)return;
    std::vector<uint8_t> rgb;
    for(uint16_t v:tft.pixelsRGB565()) {
        rgb.push_back(((v>>11)&31)*255/31);rgb.push_back(((v>>5)&63)*255/63);rgb.push_back((v&31)*255/31);
    }
    char path[1024];snprintf(path,sizeof path,"%s/%s.png",dir,name);
    ck("screenshot written",PngWriter::write(path,tft.width(),tft.height(),rgb.data()));
}
static void testDetection(DetectionType type,unsigned id) {
    Detection d{};d.type=type;d.mac[0]=0xAC;d.mac[5]=id;d.vendor="Test radio";
    d.rssi=-60;d.conf=Confidence::LOW_CONF;d.evidence=MatchEvidence::OUI;
    d.active=true;d.hits=1;d.firstSeen=d.lastSeen=millis();engine.postBle(d);
}
static int runDnspUiChecks() {
    suite("Board backlight pin isolation");
    ck("resistive CYD backlight only on GPIO21",SimPwm::pins[0]==21&&SimPwm::bits[0]==8);
    ck("no touch pin / mixed-resolution timer backlight",SimPwm::pins[1]==0&&SimPwm::pins[2]==0);
    suite("Splash visible duration and attribution");
    testStep(1);const uint32_t firstVisible=millis();
    ck("first displayed frame is splash",state==AppState::BOOT);
    testStep(6999);ck("splash remains for seven visible seconds",state==AppState::BOOT);
    TFT_eSPI footer(tft.width(),tft.height());TFT_eSprite footerFrame(&footer);
    footerFrame.setColorDepth(8);footerFrame.createSprite(tft.width(),tft.height());
    uiBootAttribution(footerFrame);footerFrame.pushSprite(0,0);
    bool matches=true;unsigned lit=0;
    for(int y=tft.height()-28;y<tft.height();y++)for(int x=0;x<tft.width();x++){
        matches &= tft.readPixel(x,y)==footer.readPixel(x,y);
        lit += footer.readPixel(x,y)!=TFT_eSprite::quantise332(Theme::BG);
    }
    ck("attribution survives real two-band splash rendering",matches&&lit>100);

    if(const char* dir=getenv("DNSP_TEST_SHOTS")){
        std::vector<uint8_t> rgb;for(uint16_t v:tft.pixelsRGB565()){rgb.push_back(((v>>11)&31)*255/31);rgb.push_back(((v>>5)&63)*255/63);rgb.push_back((v&31)*255/31);}
        char path[1024];snprintf(path,sizeof path,"%s/custom-splash.png",dir);ck("splash screenshot written",PngWriter::write(path,tft.width(),tft.height(),rgb.data()));
    }
    testStep(1);ck("normal splash exits after seven seconds",state!=AppState::BOOT&&millis()-firstVisible>=7000);
    ck("fresh rotation lock enabled",Settings::rotationLocked());
    Settings::toggleRotationLock(); // Remaining tests deliberately exercise rotation gestures.

    suite("Text continuation boundaries");
    Theme::bubbleFontOn(tft);tft.setTextSize(1);
    const char* sample="one two three four five six seven eight nine ten";
    const char* rest=sample;std::string reconstructed;
    unsigned pages=0;
    while(rest && pages++<40) {
        const char* before=rest;char rows[1][48];
        auto count=Theme::wrapText(tft,rest,60,rows,1,&rest);
        ck("page makes progress",count==1 && (!rest || rest>before));
        ck("line stays inside measured width",tft.textWidth(rows[0])<=60);
        if(!reconstructed.empty())reconstructed+=' ';
        reconstructed+=rows[0];
    }
    ck("continuations preserve every word",reconstructed==sample);
    const char* utf="éééééééééééééééééééééééééééééééééééééééééééééééééé";
    rest=utf;reconstructed.clear();pages=0;
    while(rest && pages++<100) {
        const char* before=rest;char rows[1][48];
        auto count=Theme::wrapText(tft,rest,100,rows,1,&rest);
        ck("long UTF-8 word progresses",count==1 && (!rest || rest>before));
        reconstructed+=rows[0];
        ck("continuation starts at codepoint",!rest || ((unsigned char)*rest&0xc0)!=0x80);
    }
    ck("long word preserved",reconstructed==utf);
    char shortRows[1][48];Theme::wrapText(tft,sample,90,shortRows,1);
    ck("legacy overflow is marked",strstr(shortRows[0],"...")!=nullptr);
    const char* emptyRest=nullptr;
    ck("empty text creates no lines",Theme::wrapText(tft,"  ",90,shortRows,1,&emptyRest)==0 && !emptyRest);
    Theme::bubbleFontOff(tft);

    suite("Backup progress rendering");
    tft.fillScreen(Theme::BG);CareUI::drawBackupProgress(tft,25,"COPYING",0);
    ck("progress bar shows completed portion",tft.readPixel(15,68)==Theme::CYAN);
    ck("progress bar retains unfinished portion",tft.readPixel(tft.width()-20,68)==Theme::BG);
    CareUI::drawBackupProgress(tft,500,"VERIFYING",500);
    ck("progress clamps safely at full width",tft.readPixel(tft.width()-16,68)==Theme::CYAN);
    CareUI::open(CareUI::Page::BACKUP);
    CareUI::tap(50,tft.height()-110,tft.width(),tft.height(),millis(),engine);
    ck("backup is visibly pending",CareUI::working());
    CareUI::runPending(true,millis(),engine);
    ck("SD setup waits for rendered preparing frame",CareUI::working());
    CareUI::draw(tft,millis(),engine);CareUI::runPending(true,millis(),engine);
    ck("unavailable simulator SD exits pending with result",!CareUI::working());
    CareUI::open();
    suite("Real loop: display timer starts when each card opens");
    state=AppState::CLEAR;engine.clearLog();testStep();
    if (getenv("DNSP_UI_PORTRAIT")) testTap(tft.width()-14,10);
    Settings::markTimeZoneChosen(); // Keep the first-boot timezone modal out of the dialogue scene.
    const uint8_t watched[6]={1,2,3,4,5,6};
    engine.watchBle(watched,"Dialogue check");
    Squachy::announce("A long conversation should preserve every word, stay clear of the watch controls, and give you time to read each continuation before it disappears.");
    testShot("speech-watch-first");testStep(4600);testShot("speech-watch-next");
    Squachy::announce("   ");testShot("speech-empty-rejected");
    engine.clearWatch();

    testDetection(DetectionType::FLOCK,1);testStep();
    ck("fresh detection opens alert",state==AppState::ALERT);
    const uint32_t first=alertStart;
    testDetection(DetectionType::AIRTAG,2);testStep();
    ck("second event does not replace current",lastAlertType==DetectionType::FLOCK && alertStart==first);
    testShot("queued-alert");
    testStep(15000);ck("default survives fifteen seconds",state==AppState::ALERT);
    testStep(15100);testStep();
    ck("queued card gets a fresh timer",state==AppState::ALERT && lastAlertType==DetectionType::AIRTAG && alertStart>first+30000);
    testStep(15000);ck("next card retains its own full duration",state==AppState::ALERT);

    suite("Every selectable duration");
    for(unsigned seconds:{45,60,15,30}) {
        Settings::cycleAlertSeconds();engine.clearLog();state=AppState::CLEAR;
        testDetection(DetectionType::FLOCK,seconds);testStep();
        for (unsigned wait=0; wait<3 && state!=AppState::ALERT; ++wait) testStep();
        if (state==AppState::OUTFIT_UNLOCK) { testStep(12001); testStep(); }
        ck("queued detection survives a system notice",state==AppState::ALERT);
        SimClock::nowMs=alertStart+seconds*1000-1;loop();
        ck("card remains before chosen deadline",state==AppState::ALERT);
        SimClock::nowMs=alertStart+seconds*1000+1;loop();
        ck("card dismisses after chosen deadline",state==AppState::CLEAR);
    }
    suite("User-opened evidence is readable without timeout");
    engine.clearLog();state=AppState::CLEAR;testDetection(DetectionType::FLOCK,9);testStep();testStep();
    testTap(tft.width()-45,tft.height()-30);testShot("why-matched");
    testStep(60000);ck("reading holds the alert open",state==AppState::ALERT);
    // Why -> primer -> device explanation -> external resources -> return.
    const int panelHeight = tft.height()-8 < 260 ? tft.height()-8 : 260;
    const int infoButtonY = (tft.height()-panelHeight)/2 + panelHeight - 21;
    for(unsigned i=0;i<3;++i)testTap(tft.width()/2,infoButtonY);
    testShot("camera-resources");
    testTap(tft.width()/2,infoButtonY);
    ck("resources close back to main screen",state==AppState::CLEAR);

    suite("Walkthrough navigation");
    state=AppState::DNSP_INFO;testStep();testShot("walkthrough");
    testTap(tft.width()-50,tft.height()-25);ck("next stays in walkthrough",state==AppState::DNSP_INFO);
    testShot("walkthrough-page-2");
    testTap(40,tft.height()-25);ck("skip returns to settings",state==AppState::SETTINGS);

    suite("Locked notification policy");
    Security::setPinLength(Security::PinLen::FOUR);Security::setPin("1234");
    while(Security::lockAlerts()!=Security::LockAlerts::NONE) Security::cycleLockAlerts();
    Security::lock();state=AppState::LOCKED;engine.clearLog();
    testDetection(DetectionType::FLOCK,20);testStep();
    ck("NONE cannot expose a queued alert",state==AppState::LOCKED);
    while(Security::lockAlerts()!=Security::LockAlerts::TYPE_ONLY) Security::cycleLockAlerts();
    testStep();ck("TYPE ONLY permits redacted alert",state==AppState::ALERT);
    testShot("locked-alert");testStep();testTap(tft.width()-45,tft.height()-30);
    ck("locked Why button cannot open evidence or unlock",state==AppState::LOCKED && Security::locked());
    Security::check("1234",millis());

    suite("Updater warning precedes controls");
    state=AppState::UPDATE;uiUpdateInit(tft);testStep();testShot("update-warning");
    ck("warning blocks unrelated control taps",uiUpdateHitTest(tft,tft.width()/2,80,nullptr)==UpdateHit::NONE);

    suite("Research controls and local-only exports");
    Research::discard();Research::setSink([](const char*,const char*,bool){return true;});
    state=AppState::RESEARCH;ResearchUI::open();testStep();testShot("research-setup");
    ck("current mode explicitly redacted",!strcmp(ResearchUI::modeLabel(),"CURRENT MODE: REDACTED"));
    ck("button describes next action",!strcmp(ResearchUI::modeAction(),"SWITCH TO RAW..."));
    testTap(tft.width()/2,tft.height()-80);testShot("research-raw-confirmation");
    ck("pending confirmation does not change current mode",!strcmp(ResearchUI::modeLabel(),"CURRENT MODE: REDACTED"));
    testTap(tft.width()/2,tft.height()-80);
    ResearchUI::tap(tft.width()/2,tft.height()-52,tft.width(),tft.height(),millis(),true,123);
    testStep();ck("explicit raw confirmation permits raw session",Research::active()&&Research::stats().raw);
    ck("active mode explicitly raw",!strcmp(ResearchUI::modeLabel(),"CURRENT MODE: RAW"));
    ck("mode locked during recording",!strcmp(ResearchUI::modeAction(),"MODE LOCKED WHILE RECORDING"));
    char runningLine[96];ResearchUI::sessionLine(runningLine,sizeof runningLine,Research::stats().start+65000);
    ck("recording line contains elapsed timer",strstr(runningLine,"REC 01:05")!=nullptr);
    ResearchUI::tap(tft.width()/2,tft.height()-80,tft.width(),tft.height(),millis(),true,123);
    ck("recording ignores mode changes",Research::stats().raw);
    testShot("research-recording");
    const uint8_t mac[]={0xb4,0x1e,0x52,1,2,3}, adv[]={3,0xff,0x4d,3};
    Research::observe(0,mac,0,-60,0,adv,sizeof adv,millis(),Research::matchBle(adv,sizeof adv));testStep();
    testTap(tft.width()-40,tft.height()-20);testShot("research-coverage");
    testTap(tft.width()-40,tft.height()-20);testShot("research-notebook");
    testTap(tft.width()/2,tft.height()-80);testStep();ck("visual annotation saves separately",Research::stats().saved==2);
    testTap(tft.width()-40,tft.height()-20);testShot("research-catalog");
    testTap(tft.width()-40,tft.height()-20);testShot("research-support");
    testTap(tft.width()-40,tft.height()-20);testShot("research-resources");
    testTap(tft.width()-40,tft.height()-20);testShot("research-records");
    Research::stop();ResearchUI::sessionLine(runningLine,sizeof runningLine,millis());
    ck("stopped session says saving until drained",strstr(runningLine,"SAVING")!=nullptr);
    ck("mode locked during saving",!strcmp(ResearchUI::modeAction(),"MODE LOCKED WHILE SAVING"));
    testTap(40,tft.height()-20);testStep();ck("back stops research and returns to settings",state==AppState::SETTINGS&&!Research::active());
    state=AppState::RESEARCH;ResearchUI::open();Research::start(Research::Profile::BALANCED,false,60000,millis(),124,true);
    Security::lock();testStep();ck("lock hides research and stops capture",state==AppState::LOCKED&&!Research::active());
    Security::check("1234",millis());Research::discard();

    suite("Field tools: layout, persistence, hidden language and accessibility");
    Field::config=Field::Config{};Field::reset();engine.clearLog();state=AppState::FIELD_TOOLS;FieldUI::open();testStep();
    auto fieldOpen=[&](){FieldUI::open();state=AppState::FIELD_TOOLS;testStep(400);};
    auto rowTap=[&](int row){testTap(tft.width()/2,40+row*((tft.height()-88)/4)+8);};
    ck("pit board opens",FieldUI::currentPage()==2);
    testTap(tft.width()-30,48);ck("pilot channel changed",Field::config.channels[0]==1);
    Field::config.channels[1]=1;testShot("fpv-pit-board");ck("channel collision",Field::conflict(0,1));
    Field::save();Field::config.channels[0]=0;Field::begin();ck("pit channels survive settings reload",Field::config.channels[0]==1);
    suite("DNSP tools: visible randomizer motion and compact timer layout");
    fieldOpen();FieldUI::openPage(FieldUI::RANDOMIZER);testStep();
    testTap(tft.width()/4,tft.height()-66);const auto flipStartDraws=FieldUI::drawCount();
    testStep(300);ck("coin flip redraws while moving",FieldUI::drawCount()>flipStartDraws);testShot("coin-flipping");
    testStep(800);testShot("coin-result");
    testTap(3*tft.width()/4,tft.height()-66);testStep(300);testShot("dowsing-spinning");testStep(800);testShot("dowsing-result");
    fieldOpen();FieldUI::openPage(FieldUI::TIMER_COUNTER);testStep();testShot("timer-counter");
    testTap(tft.width()-24,tft.height()-68);ck("counter controls do not trigger footer",state==AppState::FIELD_TOOLS);
    testTap(30,tft.height()-24);ck("timer footer remains independently tappable",state==AppState::SETTINGS);
    suite("Drone tools: focus, capture consent, exit and failure feedback");
    fieldOpen();FieldUI::openPage(FieldUI::DRONES);testStep();ck("drone readings reachable",FieldUI::currentPage()==3);
    testTap(tft.width()/2,tft.height()-66);ck("drone tools reachable",FieldUI::currentPage()==11);
    rowTap(0);ck("focus toggled on",DroneWatch::focused());testShot("drone-tools");
    rowTap(1);ck("diagnostics reachable",FieldUI::currentPage()==12);testShot("rid-diagnostics");
    testTap(30,tft.height()-24);rowTap(2);ck("capture reachable",FieldUI::currentPage()==13);
    DroneWatch::setSink([](const char*,bool){return true;});
    testTap(tft.width()/2,tft.height()-66);ck("raw capture requires second tap",DroneWatch::settled());testShot("rid-capture-consent");
    testTap(tft.width()/2,tft.height()-66);ck("preparing is displayed first",DroneWatch::stats().capture==DroneWatch::Capture::STARTING);testStep();ck("capture running",DroneWatch::stats().capture==DroneWatch::Capture::RECORDING);
    testStep(2300);testShot("rid-recording");
    testTap(30,tft.height()-24);testStep();ck("capture finishes on back",DroneWatch::settled());
    rowTap(2);DroneWatch::setSink([](const char*,bool){return false;});
    testTap(tft.width()/2,tft.height()-66);testTap(tft.width()/2,tft.height()-66);
    testStep();ck("SD failure visible",DroneWatch::stats().capture==DroneWatch::Capture::ERROR);testShot("rid-error");
    DroneWatch::setSink([](const char*,bool){return true;});
    testTap(tft.width()/2,tft.height()-66);testTap(tft.width()/2,tft.height()-66);
    Security::lock();testStep();ck("lock stops focus and capture",!DroneWatch::focused()&&DroneWatch::settled());
    Security::check("1234",millis());DroneWatch::setSink(nullptr);
    fieldOpen();FieldUI::openLanguage();testStep();ck("language opens",FieldUI::currentPage()==8);
    ck("Hebrew hidden initially",!Field::config.hebrew);
    for(int i=0;i<6;i++)Lang::next();ck("normal cycle excludes Hebrew",Field::config.language==0);
    for(int i=0;i<7;i++)testTap(tft.width()/2,24);ck("title taps no longer unlock",!Field::config.hebrew);
    testTap(tft.width()/2,74);ck("short language tap cycles",Field::config.language==1);
    auto languageDown=[&](){testStep(300);screenToRaw(tft.width()/2,74,SimTouch::rawX,SimTouch::rawY);SimTouch::down=true;testStep();};
    languageDown();testStep(2900);ck("hold under three seconds stays hidden",!Field::config.hebrew);
    screenToRaw(tft.width()/2,115,SimTouch::rawX,SimTouch::rawY);testStep(200);
    SimTouch::down=false;testStep();ck("drag away cancels without cycling",!Field::config.hebrew&&Field::config.language==1);
    languageDown();testStep(1200);SimTouch::down=false;testStep();
    ck("incomplete hold neither unlocks nor cycles",!Field::config.hebrew&&Field::config.language==1);
    languageDown();testStep(1500);FieldUI::openLanguage();testStep(1600);
    SimTouch::down=false;testStep();ck("reopening cancels held gesture",!Field::config.hebrew&&Field::config.language==1);
    transitionStart=millis();testStep(33);const uint32_t drawsDuringTransition=FieldUI::drawCount();
    testStep(700);ck("slow frame repairs cached transition",FieldUI::drawCount()>drawsDuringTransition);
    languageDown();testStep(2999);ck("2999 ms stays hidden",!Field::config.hebrew);
    testStep(1);ck("three-second hold reveals",Field::config.hebrew&&Field::config.language==1);
    testStep(1000);SimTouch::down=false;testStep();ck("hold release does not cycle",Field::config.language==1);
    for(int i=0;i<7;i++){Field::config.language=i;testShot(i==0?"language-english":i==1?"language-spanish":i==2?"language-french":i==3?"language-german":i==4?"language-japanese":i==5?"language-chinese":"language-hebrew");}
    Field::save();Field::config.language=0;Field::begin();ck("Hebrew persists when selected",Field::config.language==6&&Field::config.hebrew);
    FieldUI::openHelp();testShot("hebrew-help");
    Field::config.language=4;FieldUI::openHelp();testShot("japanese-help");
    Field::config.language=5;FieldUI::openHelp();testShot("chinese-help");
    Field::config.language=1;state=AppState::SETTINGS;uiSettingsInit(tft);testShot("spanish-settings");
    Field::config.language=0;fieldOpen();FieldUI::openAccessibility();testStep();rowTap(0);rowTap(1);rowTap(3);
    ck("contrast, motion and controls switch",Field::config.contrast&&Field::config.reduced&&Field::config.large);
    ck("large shared navigation",Theme::computeButtonBar(tft.width(),tft.height()).h>=34);
    rowTap(2);ck("left handed toggles",Field::config.left);testShot("accessibility");
    testTap(tft.width()-30,tft.height()-24);ck("mirrored back returns to settings",state==AppState::SETTINGS);
    state=AppState::FIELD_TOOLS;FieldUI::openAccessibility();testStep();
    Security::lock();testStep();ck("lock hides field data",state==AppState::LOCKED);Security::check("1234",millis());
    Field::config=Field::Config{};Field::save();Theme::applyPalette(Settings::paletteIndex());
    state=AppState::FIELD_TOOLS;FieldUI::open();testStep(1000);testStep(1000);
    const auto initialRenders=FieldUI::drawCount();for(int i=0;i<20;i++)testStep(33);
    ck("static field page avoids repeated redraws",FieldUI::drawCount()==initialRenders);
    Field::config.language=2;testStep();ck("language change invalidates cached screen",FieldUI::drawCount()>initialRenders);
    Field::config.language=0;
    suite("Breakout through real menu, touch, alert queue and lock");
    engine.clearLog();engine.clearWatch();state=AppState::SETTINGS;uiSettingsInit(tft);testStep(400);
    auto menuTap = [&](SettingsRow wanted) {
        for(int scroll=0;scroll<80;++scroll){
            for(int y=34;y<tft.height()-Theme::pinnedBackH(tft.width());++y){
                if(uiSettingsHitTest(tft,tft.width()/2,y,tft.width(),tft.height())==wanted){
                    testTap(tft.width()/2,y+8);return true;
                }
            }
            uiSettingsScroll(1);testStep();
        }
        return false;
    };
    testShot("menu-main");
    ck("system menu opens",menuTap(SettingsRow::SYSTEM));
    ck("system info reachable",menuTap(SettingsRow::SYSTEM_INFO)&&state==AppState::SYS_PROPS);
    const int windowW=tft.width()-8<300?tft.width()-8:300;
    const int windowX=(tft.width()-windowW)/2;
    const int windowH=tft.height()-8<232?tft.height()-8:232;
    const int tabY=(tft.height()-windowH)/2+24;
    testTap(windowX+windowW-30,tabY+8);testShot("credits");
    const int propsY=(tft.height()-windowH)/2;
    auto creditPixels=[&](){uint32_t h=2166136261u;for(int y=propsY+49;y<propsY+windowH-38;++y)for(int x=windowX+12;x<windowX+windowW-12;++x)h=(h^tft.readPixel(x,y))*16777619u;return h;};
    const auto creditTop=creditPixels();
    testTap(windowX+52,propsY+windowH-18);testStep(200);
    ck("credits Down scrolls when needed",tft.width()>240 || creditPixels()!=creditTop);
    testTap(windowX+30,propsY+windowH-18);testStep(200);
    ck("credits Up restores top",creditPixels()==creditTop);
    for(int i=0;i<60;++i)testTap(windowX+52,propsY+windowH-18);
    testShot("credits-bottom");
    testTap(windowX+windowW-12,(tft.height()-windowH)/2+10);
    ck("info close returns to settings",state==AppState::SETTINGS);
    ck("info returns to System category",uiSettingsCurrentPage()==SettingsPage::SYSTEM);
    ck("color setup reachable",menuTap(SettingsRow::CHECK_COLORS)&&state==AppState::COLOR_CHECK);
    int invX=-1,invY=-1;
    for(int y=0;y<tft.height() && invX<0;++y)for(int x=0;x<tft.width();++x)
        if(uiColorCheckHitTest(x,y,tft.width(),tft.height())==ColorCheckTap::INVERT){invX=x+4;invY=y+4;break;}
    const bool inversionBefore=Settings::inverted();
    screenToRaw(invX,invY,SimTouch::rawX,SimTouch::rawY);SimTouch::down=true;testStep();
    ck("color toggles on initial press",Settings::inverted()!=inversionBefore);
    for(int i=0;i<15;++i)testStep(200);
    ck("held color touch does not repeat",Settings::inverted()!=inversionBefore);
    SimTouch::down=false;testStep();testTap(invX,invY);
    ck("second press toggles once",Settings::inverted()==inversionBefore);
    state=AppState::SETTINGS;

    uiSettingsInit(tft);testStep();

    ck("language now under System",menuTap(SettingsRow::SYSTEM)&&uiSettingsCurrentPage()==SettingsPage::SYSTEM);
    ck("direct language entry",menuTap(SettingsRow::LANGUAGE)&&FieldUI::currentPage()==8);
    testTap(20,tft.height()-24);ck("language back returns to settings",state==AppState::SETTINGS);
    ck("direct accessibility entry",menuTap(SettingsRow::ACCESSIBILITY)&&FieldUI::currentPage()==7);
    testTap(20,tft.height()-24);ck("accessibility back returns to settings",state==AppState::SETTINGS);
    uiSettingsOpenPage(SettingsPage::MAIN);testStep();
    ck("alerts category opens",menuTap(SettingsRow::ALERTS)&&uiSettingsCurrentPage()==SettingsPage::ALERTS);
    testShot("menu-alerts");
    suite("Snooze All controls and retained detections");
    ck("global snooze reachable from alert settings",menuTap(SettingsRow::SNOOZE_ALL)&&AlertSnooze::active(millis()));
    testShot("snooze-settings");
    ck("resume control works",menuTap(SettingsRow::SNOOZE_ALL)&&!AlertSnooze::active(millis()));
    engine.clearLog();state=AppState::CLEAR;testStep();
    testDetection(DetectionType::FLOCK,211);testDetection(DetectionType::AIRTAG,212);testStep();
    ck("burst opens alert",state==AppState::ALERT);testShot("snooze-all-popup");
    testTap(tft.width()-60,tft.height()-60);
    ck("popup snoozes whole burst",AlertSnooze::active(millis())&&state==AppState::CLEAR);
    auto count=engine.logCount();testDetection(DetectionType::DRONE,213);testStep();
    ck("detection log continues without popup",engine.logCount()>count&&state==AppState::CLEAR);
    testShot("snoozed-scanning");AlertSnooze::resume();engine.alerts.clear();
    testDetection(DetectionType::AIRTAG,214);testStep();ck("fresh alert resumes normally",state==AppState::ALERT);
    engine.clearLog();state=AppState::SETTINGS;uiSettingsInit(tft);uiSettingsOpenPage(SettingsPage::ALERTS);testStep();
    ck("stored alert history directly reachable",menuTap(SettingsRow::ALERT_HISTORY)&&FieldUI::currentPage()==FieldUI::ALERT_HISTORY_SCAN);
    testTap(20,tft.height()-24);ck("alert history back returns to alert settings",state==AppState::SETTINGS&&uiSettingsCurrentPage()==SettingsPage::ALERTS);
    ck("detection profiles directly reachable",menuTap(SettingsRow::DETECTION_PROFILE)&&FieldUI::currentPage()==FieldUI::SCAN_PROFILES);
    testTap(20,tft.height()-24);ck("profiles back returns to alert settings",state==AppState::SETTINGS&&uiSettingsCurrentPage()==SettingsPage::ALERTS);
    ck("advanced alert rules reachable",menuTap(SettingsRow::ALERT_RULES)&&FieldUI::currentPage()==6);
    testTap(20,tft.height()-24);ck("rules back returns to settings",state==AppState::SETTINGS);
    // Enumerate rendered/hit-tested rows in every page, including the bottom.
    auto reachable = [&](SettingsPage page, SettingsRow row) {
        uiSettingsInit(tft);uiSettingsOpenPage(page);testStep();
        for(int scroll=0;scroll<80;++scroll){
            for(int y=34;y<tft.height()-Theme::pinnedBackH(tft.width());++y)
                if(uiSettingsHitTest(tft,tft.width()/2,y,tft.width(),tft.height())==row)return true;
            uiSettingsScroll(1);testStep();
        }
        return false;
    };
    const struct {SettingsPage page;SettingsRow row;} destinations[]={
        {SettingsPage::MAIN,SettingsRow::DNSP_MENU},{SettingsPage::MAIN,SettingsRow::DATA_MENU},
        {SettingsPage::SYSTEM,SettingsRow::CREDITS},{SettingsPage::SYSTEM,SettingsRow::DEVICE_HELP},{SettingsPage::SYSTEM,SettingsRow::CRASH_REPORTS},{SettingsPage::APPEARANCE,SettingsRow::AMBIENT_LIGHT},{SettingsPage::MAIN,SettingsRow::STORAGE_MENU},
        {SettingsPage::FUN,SettingsRow::OUTFIT},{SettingsPage::FUN,SettingsRow::BINGO},{SettingsPage::FUN,SettingsRow::DEX},
        {SettingsPage::DNSP,SettingsRow::BREAKOUT},{SettingsPage::DNSP,SettingsRow::READABLE_LOGS},{SettingsPage::DNSP,SettingsRow::DNSP_GUIDE},
        {SettingsPage::DNSP,SettingsRow::PRACTICE},{SettingsPage::DNSP,SettingsRow::GIFT_PREP},
        {SettingsPage::FPV,SettingsRow::FPV_PIT},{SettingsPage::FPV,SettingsRow::DRONE_READINGS},
        {SettingsPage::FPV,SettingsRow::DRONE_SEARCH},{SettingsPage::FPV,SettingsRow::DRONE_DIAG},
        {SettingsPage::FPV,SettingsRow::DRONE_CAPTURE},{SettingsPage::FPV,SettingsRow::DRONE_LIMITS},
        {SettingsPage::DATA,SettingsRow::RESEARCH},{SettingsPage::DATA,SettingsRow::FIELD_REPORT},
        {SettingsPage::DATA,SettingsRow::TELEMETRY},{SettingsPage::DATA,SettingsRow::SENSORS},
        {SettingsPage::SYSTEM,SettingsRow::ACCESSIBILITY},{SettingsPage::SYSTEM,SettingsRow::LANGUAGE},
        {SettingsPage::STORAGE,SettingsRow::BACKUP},{SettingsPage::SYSTEM,SettingsRow::DEVICE_HEALTH},
        {SettingsPage::SYSTEM,SettingsRow::POWER_SAVER},{SettingsPage::ALERTS,SettingsRow::TROUBLESHOOT},
    };
    for(const auto& d:destinations)ck("approved destination is reachable",reachable(d.page,d.row));
    ck("favorites removed from root",!reachable(SettingsPage::MAIN,SettingsRow::QUICK_MENU));
    ck("old help hub removed from root",!reachable(SettingsPage::MAIN,SettingsRow::CARE));
    ck("Breakout separated from Squachy",!reachable(SettingsPage::FUN,SettingsRow::BREAKOUT));

    suite("Bounded scan profiles and automatic recovery");
    ScanProfile::restore(ScanProfile::BALANCED,3900,75,30);
    ScanProfile::setCustomWifi(1);ck("custom WiFi clamps to two seconds",ScanProfile::customWifiMs()==2000);
    ScanProfile::setCustomWifi(9999);ck("custom WiFi clamps to eight seconds",ScanProfile::customWifiMs()==8000);
    ScanProfile::setCustomBle(1);ck("custom Bluetooth clamps to forty percent",ScanProfile::customBleShare()==40);
    ScanProfile::setCustomBle(99);ck("custom Bluetooth clamps to eighty-five percent",ScanProfile::customBleShare()==85);
    ScanProfile::select(ScanProfile::MAXIMUM,100);
    ScanProfile::tick(1800100,0,0,0,0,0,12000);
    ck("Maximum returns to Balanced after thirty minutes",ScanProfile::current()==ScanProfile::BALANCED);
    ScanProfile::startComparison(1000,10,20,1,2,3);
    ScanProfile::tick(46000,20,30,2,3,4,11000);
    ScanProfile::tick(91000,30,40,3,4,5,10000);
    ScanProfile::tick(136000,40,50,4,5,6,9000);
    ScanProfile::tick(181000,50,60,5,6,7,8000);
    ck("four-profile comparison completes",ScanProfile::comparisonDone()&&!ScanProfile::comparing());
    ck("comparison restores prior profile",ScanProfile::current()==ScanProfile::BALANCED);
    for(auto pg:{SettingsPage::MAIN,SettingsPage::FUN,SettingsPage::DNSP,SettingsPage::FPV,SettingsPage::DATA,SettingsPage::SYSTEM,SettingsPage::STORAGE}) {
        uiSettingsInit(tft);uiSettingsOpenPage(pg);testStep();
        char label[32];snprintf(label,sizeof label,"new-menu-%u",unsigned(pg));testShot(label);
    }
    suite("v1.2 location and Remington");
    uiSettingsInit(tft);uiSettingsOpenPage(SettingsPage::ALERTS);testStep();
    ck("location menu reachable",menuTap(SettingsRow::SET_LOCATION)&&state==AppState::LOCATION_LABEL);
    testTap(40,106);ck("Home preset applies",!strcmp(LocationLabel::current(),"Home"));testShot("location-home");
    testTap(40,74);ck("label keyboard opens",state==AppState::LOCATION_EDIT);testShot("location-keyboard");
    state=AppState::LOCATION_LABEL;testStep();testTap(tft.width()-40,74);ck("clear label",!strcmp(LocationLabel::current(),"no-label-set"));
    testTap(40,tft.height()-18);ck("location back returns to settings",state==AppState::SETTINGS);
    uiSettingsOpenPage(SettingsPage::DNSP);testStep();
    ck("Remington menu reachable",menuTap(SettingsRow::REMINGTON)&&state==AppState::REMINGTON);testShot("remington-photo");
    testTap(100,100);testTap(100,100);ck("Remington double tap returns",state==AppState::SETTINGS);
    suite("Device help, credits, light setting and crash reader");
    uiSettingsInit(tft);uiSettingsOpenPage(SettingsPage::SYSTEM);testStep();
    ck("direct Credits entry opens",menuTap(SettingsRow::CREDITS)&&state==AppState::SYS_PROPS);testShot("direct-credits");
    testTap(windowX+windowW-12,propsY+10);ck("credits returns to System",uiSettingsCurrentPage()==SettingsPage::SYSTEM);
    ck("device help reachable",menuTap(SettingsRow::DEVICE_HELP)&&state==AppState::DEVICE_READER);testShot("device-help");
    testTap(40,90);testShot("power-help");
    for(int i=0;i<4;++i)testTap(tft.width()/2,tft.height()-25);testShot("power-help-end");
    testTap(tft.width()-25,tft.height()-25);testTap(40,130);testShot("data-help-no-card");
    testTap(tft.width()-25,tft.height()-25);testTap(40,tft.height()-20);
    ck("help returns to System",state==AppState::SETTINGS&&uiSettingsCurrentPage()==SettingsPage::SYSTEM);
    ck("crash menu reachable",menuTap(SettingsRow::CRASH_REPORTS)&&state==AppState::DEVICE_READER);testShot("crash-no-card");
    testTap(40,tft.height()-20);
    uiSettingsOpenPage(SettingsPage::APPEARANCE);testStep();bool wasAmbient=Settings::ambientLight();
    ck("light sensor toggle reachable",menuTap(SettingsRow::AMBIENT_LIGHT)&&Settings::ambientLight()!=wasAmbient);testShot("light-setting");
    ck("sensor toggle can be turned back off",menuTap(SettingsRow::AMBIENT_LIGHT)&&Settings::ambientLight()==wasAmbient);
    if(getenv("DNSP_TEST_SD")){
        CrashReports::Record rec;rec.resetReason=4;rec.crash.valid=true;rec.crash.uptimeMs=12500;rec.crash.pc=0x40001234;
        snprintf(rec.firmware,sizeof rec.firmware,"TEST FIXTURE");ck("queue simulated crash",CrashReports::enqueue(rec));CrashReports::service(true,millis()+6000);
        DeviceUI::open(DeviceUI::REPORTS,true,tft.width(),tft.height());state=AppState::DEVICE_READER;testShot("crash-list");
        testTap(80,45);testShot("crash-reader");testTap(tft.width()/2,tft.height()-25);testShot("crash-reader-next");
        Security::lock();testStep();ck("PIN lock hides crash reader",state==AppState::LOCKED);Security::check("1234",millis());
        state=AppState::SETTINGS;uiSettingsInit(tft);testStep();
    }
    state=AppState::SYS_PROPS;uiSysPropsInit(tft);testStep();testShot("update-info");
    for(int i=0;i<20;++i){uiSysPropsScroll(1);testStep();}testShot("update-info-bottom");
    state=AppState::SETTINGS;uiSettingsInit(tft);testStep();
    ck("shutdown reachable",reachable(SettingsPage::MAIN,SettingsRow::POWER_CONTROL));
    ck("storage reachable",reachable(SettingsPage::STORAGE,SettingsRow::SD_STATUS));
    ck("security reachable",reachable(SettingsPage::MAIN,SettingsRow::SECURITY));
    ck("ignored devices reachable",reachable(SettingsPage::ALERTS,SettingsRow::IGNORED_DEVICES));
    ck("status light reachable",reachable(SettingsPage::APPEARANCE,SettingsRow::STATUS_LIGHT));
    ck("reset reachable",reachable(SettingsPage::SYSTEM,SettingsRow::RESET_STATS));
    ck("display speed reachable",reachable(SettingsPage::SYSTEM,SettingsRow::DISPLAY_SPEED));
    const uint8_t oldSpeed=Settings::displayMhz();
    ck("display speed tap works",menuTap(SettingsRow::DISPLAY_SPEED)&&Settings::displayMhz()!=oldSpeed);
    Settings::load();ck("display speed survives reload",Settings::displayMhz()!=oldSpeed);
    testShot("menu-display-speed");
    ck("display speed toggles back",menuTap(SettingsRow::DISPLAY_SPEED)&&Settings::displayMhz()==oldSpeed);
    ck("clock settings reachable",reachable(SettingsPage::DESK,SettingsRow::TIME_ZONE));
    ck("boring mode reachable",reachable(SettingsPage::FUN,SettingsRow::BORING_MODE));
    uiSettingsInit(tft);uiSettingsOpenPage(SettingsPage::FUN);testStep();
    ck("settings headings are labels",uiSettingsHitTest(tft,20,35,tft.width(),tft.height())==SettingsRow::NONE);
    uiSettingsOpenPage(SettingsPage::MAIN);testStep();
    ck("folding subpage leaves main games entry visible",menuTap(SettingsRow::FUN)&&uiSettingsCurrentPage()==SettingsPage::FUN);
    uiSettingsInit(tft);testStep();
    ck("DNSP category opens",menuTap(SettingsRow::DNSP_MENU)&&uiSettingsCurrentPage()==SettingsPage::DNSP);
    testShot("menu-games");
    bool foundGame=false;
    for(int scroll=0;scroll<32&&!foundGame;++scroll){
        for(int y=36;y<tft.height()-44;++y){
            if(uiSettingsHitTest(tft,tft.width()/2,y,tft.width(),tft.height())==SettingsRow::BREAKOUT){
                testTap(tft.width()/2,y+8);foundGame=true;break;
            }
        }
        if(!foundGame){uiSettingsScroll(1);testStep();}
    }
    ck("menu opens Breakout",foundGame&&state==AppState::BREAKOUT);
    testStep(400);testShot("breakout-ready");
    testTap(tft.width()/2, tft.height()/2);
    ck("tap launches ball",BreakoutUI::game().phase==Breakout::Phase::PLAYING);
    screenToRaw(tft.width()-24,tft.height()/2,SimTouch::rawX,SimTouch::rawY);SimTouch::down=true;testStep();
    ck("held drag moves paddle",BreakoutUI::game().paddle>260);
    testDetection(DetectionType::FLOCK,81);testStep();
    const auto frozenGame=BreakoutUI::game();
    ck("detection interrupts and pauses game",state==AppState::ALERT&&frozenGame.phase==Breakout::Phase::PAUSED);
    testStep(500);ck("held steering finger cannot dismiss alert",state==AppState::ALERT);
    SimTouch::down=false;testStep();
    testDetection(DetectionType::AIRTAG,82);testStep();
    testStep(61000);testStep();
    ck("queued alert still interrupts resumed screen",state==AppState::ALERT&&lastAlertType==DetectionType::AIRTAG);
    testStep(61000);testStep(400);
    ck("alert timeout returns to paused game",state==AppState::BREAKOUT&&BreakoutUI::game().phase==Breakout::Phase::PAUSED);
    ck("alerts preserve ball score and lives",BreakoutUI::game().x==frozenGame.x&&BreakoutUI::game().y==frozenGame.y&&BreakoutUI::game().lives==frozenGame.lives&&BreakoutUI::game().score==frozenGame.score);
    testShot("breakout-paused");
    testTap(tft.width()/2,tft.height()-20);ck("resume requires user action",BreakoutUI::game().phase==Breakout::Phase::PLAYING);
    testTap(tft.width()-25,tft.height()-20);ck("new round resets score and lives",BreakoutUI::game().phase==Breakout::Phase::READY&&BreakoutUI::game().score==0&&BreakoutUI::game().lives==3);
    const bool oldSaver=Settings::powerSaver();
    const uint16_t oldTimeout=Settings::screenTimeoutSecRaw();
    if(!oldSaver)Settings::togglePowerSaver();
    if(!Settings::screenTimeoutSec())Settings::cycleScreenTimeout();
    testTap(tft.width()/2,tft.height()/2);
    lastTouch=millis();testStep(uint32_t(Settings::screenTimeoutSec())*1000+1);
    ck("screen timeout pauses game",BreakoutUI::game().phase==Breakout::Phase::PAUSED);
    for(int restore=0;restore<20&&Settings::screenTimeoutSecRaw()!=oldTimeout;++restore)Settings::cycleScreenTimeout();
    if(!oldSaver)Settings::togglePowerSaver();
    Security::lock();testStep();ck("lock hides and pauses game",state==AppState::LOCKED&&BreakoutUI::game().phase==Breakout::Phase::PAUSED);
    Security::check("1234",millis());state=AppState::BREAKOUT;BreakoutUI::open(millis());testStep(400);
    testTap(20,tft.height()-20);ck("back returns to settings",state==AppState::SETTINGS);

    suite("Direct care destinations and lock");
    state=AppState::SETTINGS;uiSettingsInit(tft);testStep(400);
    ck("DNSP entry reachable",menuTap(SettingsRow::DNSP_MENU));
    ck("practice entry reachable",menuTap(SettingsRow::PRACTICE)&&state==AppState::CARE&&CareUI::page()==CareUI::Page::DEMO);
    {const bool wasRotating=TFT_eSPI::rotates;TFT_eSPI::rotates=true;int oldW=tft.width();
     testTap(tft.width()-14,10);ck("care rotation button works",tft.width()!=oldW);
     for(int i=0;i<3;i++)testTap(tft.width()-14,10);
     ck("care rotation returns",tft.width()==oldW);TFT_eSPI::rotates=wasRotating;}
    const auto beforeDemo=engine.lifetimeTotal();const auto logsBefore=engine.logCount();const auto researchBefore=Research::stats().observed;
    for(int i=0;i<4;i++){testShot("care-demo");testTap(tft.width()-30,tft.height()-24);testTap(70,65);}
    ck("demo cannot alter real history, progression or research",beforeDemo==engine.lifetimeTotal()&&logsBefore==engine.logCount()&&researchBefore==Research::stats().observed);
    testTap(20,tft.height()-24);
    ck("practice back returns to DNSP",state==AppState::SETTINGS&&uiSettingsCurrentPage()==SettingsPage::DNSP);
    state=AppState::CARE;CareUI::open(CareUI::Page::STATUS);testStep(400);testShot("care-status");
    CareUI::open(CareUI::Page::BACKUP);testShot("care-backup");
    testTap(70,tft.height()-70);testShot("care-restore-confirm");
    Security::lock();testStep();ck("lock hides care screens",state==AppState::LOCKED);Security::check("1234",millis());
    state=AppState::CARE;CareUI::open(CareUI::Page::GIFT);testStep(400);testShot("care-gift");
    CareUI::open(CareUI::Page::WELCOME);testShot("care-welcome");
    CareUI::open(CareUI::Page::HEALTH);testShot("care-health");
    CareUI::open(CareUI::Page::REPORT);testShot("care-report");
    ck("no false simulated backup success",!Backup::start(true,millis())&&!Backup::verifiedThisBoot());
    if(getenv("DNSP_SOAK")){
        suite("Accelerated mixed-screen endurance (simulator, not hardware)");
        engine.clearLog();engine.clearWatch();Settings::deskActive(false);
        if(Settings::powerSaver())Settings::togglePowerSaver();
        const auto beginLoops=Care::health().loops;
        for(unsigned i=0;i<12000;i++){
            if(i%120==0){state=AppState::BREAKOUT;BreakoutUI::open(millis());}
            if(i%120==30){state=AppState::CARE;CareUI::open(CareUI::Page::STATUS);}
            if(i%120==60){state=AppState::SETTINGS;uiSettingsInit(tft);uiSettingsOpenPage(SettingsPage::FUN);}
            if(i%120==90){state=AppState::CARE;CareUI::open(CareUI::Page::DEMO);}
            if(state==AppState::SETTINGS&&i%6==0)uiSettingsScroll(1);
            if(state==AppState::BREAKOUT&&i%5==0)BreakoutUI::input(30+i%200,90,tft.width(),tft.height(),true,true,millis());
            testStep(1000);
        }
        ck("12000 mixed-state loops completed",Care::health().loops-beginLoops==12000);
        ck("no practice observations created",engine.logCount()==0);
    }
    suite("Safe shutdown drains recording and stops application writes");
    Research::discard();Research::setSink([](const char*,const char*,bool){return true;});
    state=AppState::POWER_CONTROL;testStep(1000);testShot("shutdown-menu");
    Research::start(Research::Profile::BALANCED,false,60000,millis(),125,true);
    for(int i=0;i<10;i++)Research::observe(0,mac,0,-60,0,adv,sizeof adv,millis(),Research::matchBle(adv,sizeof adv));
    ck("shutdown begins with queued research writes",!Research::settled()&&Research::stats().saved<Research::stats().observed);
    testTap(tft.width()/2,tft.height()-(getenv("DNSP_TEST_REBOOT")?66:108));
    for(int i=0;i<20;i++)testStep();
    ck("shutdown reaches safe-off state",state==AppState::SAFE_OFF);
    ck("research queue and summary drained",Research::settled()&&!Research::active());
    ck("telemetry disconnected",!Field::telemetryActive());
    testShot("safe-off");
    TFT_eSPI expected(tft.width(),tft.height());TFT_eSprite expectedFrame(&expected);
    expectedFrame.setColorDepth(8);expectedFrame.createSprite(tft.width(),tft.height());
    expectedFrame.fillRect(0,0,tft.width(),tft.height(),Theme::BG);
    Lang::draw(expectedFrame,"microSD unmounted. Safe to power off.",12,40,tft.width()-24,tft.height()-110,Theme::WHITE);
    Lang::button(expectedFrame,12,tft.height()-52,tft.width()-24,38,"REBOOT");
    expectedFrame.pushSprite(0,0);
    ck("shutdown message and single reboot button match full screen",expected.pixelsRGB565()==tft.pixelsRGB565());
    return report();
}

// Separate run because entering duress intentionally removes access to the app.
#include "duress_device.h"
#include "ui_security.h"
#include "ui_phone.h"
extern unsigned simEngineInitCalls,simEngineLoopCalls;
static int runDuressUiChecks(bool earlyBoot) {
    if(earlyBoot)ck("early duress boot skips engine initialization",simEngineInitCalls==0);
    if(!earlyBoot){
        suite("Duress setup visibility and disclosure");
        Security::disable();
        if(Settings::rotationLocked())Settings::toggleRotationLock();
        if(getenv("DNSP_UI_PORTRAIT")){state=AppState::CLEAR;testStep();testTap(tft.width()-14,10);}
        uiSecurityInit(tft);state=AppState::SECURITY;testStep();
        auto findRow=[](SecurityRow row){
            for(int y=24;y<tft.height()-45;y++)if(uiSecurityHitTest(tft,30,y,tft.width(),tft.height())==row)return y+4;
            return -1;
        };
        ck("duress row absent with PIN disabled",findRow(SecurityRow::DURESS_PIN)<0);
        Security::setPinLength(Security::PinLen::FOUR);Security::setPin("1234");uiSecurityInit(tft);testStep();
        testStep();int y=findRow(SecurityRow::DURESS_PIN);ck("duress row available with PIN enabled",y>=0);
        testTap(30,y);testShot("duress-warning");
        ck("warning precedes PIN setup",state==AppState::SECURITY && !Security::hasDuress());
        testTap(30,tft.height()-25);ck("warning can be cancelled",state==AppState::SECURITY && !Security::hasDuress());
        testTap(30,y);testTap(tft.width()-40,tft.height()-25);
        ck("continue asks for current PIN",state==AppState::PIN_ENTRY && !Security::hasDuress());
        // Exercise actual lock-screen handling, not a direct call to the wipe.
        Security::setDuress("9999");Security::lock();engine.clearLog();
        while(Security::lockAlerts()!=Security::LockAlerts::NONE)Security::cycleLockAlerts();
        state=AppState::LOCKED;uiPhoneInitPin(tft,4,"LOCKED",false);testStep();
        for(unsigned n=0;n<4;n++)uiPhoneTouch(220,160,millis(),PhoneTouch::DOWN);
        ck("four keypad digits are accepted",uiPhonePinReady());testStep();
        ck("lock-screen duress persists intent",DuressDevice::boot()==Duress::Boot::PENDING);
    }
    suite("Isolated wipe / PIXEL TIDE loop");
    for(unsigned i=0;i<60;i++)testStep(33);
    testShot("duress-loading");
    ck("wipe completion recorded",DuressDevice::boot()==Duress::Boot::DECOY);
    for(unsigned i=0;i<230;i++)testStep(33);
    testShot("pixel-tide");
    auto before=tft.pixelsRGB565();testTap(130,110);testShot("pixel-tide-touch");
    ck("tide animates and responds to interaction",before!=tft.pixelsRGB565());
    auto loops=simEngineLoopCalls;testStep(1000);
    ck("decoy never enters the engine loop",simEngineLoopCalls==loops);
    auto inits=simEngineInitCalls;
    // Re-enter setup with the durable completion record, as on a later boot.
    setup();testStep(100);testShot("pixel-tide-reboot");
    ck("completed record still selects decoy",DuressDevice::boot()==Duress::Boot::DECOY && simEngineInitCalls==inits);
    return report();
}
