// Opt-in desktop integration checks against the real firmware setup()/loop().
#pragma once
#include "../test/test_util.h"
#include "settings.h"
#include "research.h"
#include "ui_research.h"
#include "ui_field.h"
#include "ui_breakout.h"
#include "ui_care.h"
#include "care.h"
#include "field_tools.h"
#include "language.h"
#include "theme.h"
#include "ui_settings.h"
#include "security.h"
#include "ui_update.h"
#include "png_writer.h"
extern uint32_t alertStart;
extern uint32_t lastTouch;
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
    suite("Real loop: display timer starts when each card opens");
    state=AppState::CLEAR;engine.clearLog();testStep();
    if (getenv("DNSP_UI_PORTRAIT")) testTap(tft.width()-14,10);
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
    testTap(tft.width()-50,tft.height()-25);
    ck("next stays in walkthrough",state==AppState::DNSP_INFO);
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
    testTap(tft.width()/2,tft.height()-80);testShot("research-raw-confirmation");
    testTap(tft.width()/2,tft.height()-80);
    ResearchUI::tap(tft.width()/2,tft.height()-52,tft.width(),tft.height(),millis(),true,123);
    testStep();ck("explicit raw confirmation permits raw session",Research::active()&&Research::stats().raw);
    const uint8_t mac[]={0xb4,0x1e,0x52,1,2,3}, adv[]={3,0xff,0x4d,3};
    Research::observe(0,mac,0,-60,0,adv,sizeof adv,millis(),Research::matchBle(adv,sizeof adv));testStep();
    testTap(tft.width()-40,tft.height()-20);testShot("research-coverage");
    testTap(tft.width()-40,tft.height()-20);testShot("research-notebook");
    testTap(tft.width()/2,tft.height()-80);testStep();ck("visual annotation saves separately",Research::stats().saved==2);
    testTap(tft.width()-40,tft.height()-20);testShot("research-catalog");
    testTap(tft.width()-40,tft.height()-20);testShot("research-support");
    testTap(tft.width()-40,tft.height()-20);testShot("research-resources");
    testTap(tft.width()-40,tft.height()-20);testShot("research-records");
    testTap(40,tft.height()-20);testStep();ck("back stops research and returns to settings",state==AppState::SETTINGS&&!Research::active());
    state=AppState::RESEARCH;ResearchUI::open();Research::start(Research::Profile::BALANCED,false,60000,millis(),124,true);
    Security::lock();testStep();ck("lock hides research and stops capture",state==AppState::LOCKED&&!Research::active());
    Security::check("1234",millis());Research::discard();

    suite("Field tools: layout, persistence, hidden language and accessibility");
    Field::config=Field::Config{};Field::reset();engine.clearLog();state=AppState::FIELD_TOOLS;FieldUI::open();testStep();
    auto fieldOpen=[&](){FieldUI::open();state=AppState::FIELD_TOOLS;testStep(400);};
    auto rowTap=[&](int row){testTap(tft.width()/2,40+row*((tft.height()-88)/4)+8);};
    rowTap(0);ck("pit board opens",FieldUI::currentPage()==2);
    testTap(tft.width()-30,48);ck("pilot channel changed",Field::config.channels[0]==1);
    Field::config.channels[1]=1;testShot("fpv-pit-board");ck("channel collision",Field::conflict(0,1));
    Field::save();Field::config.channels[0]=0;Field::begin();ck("pit channels survive settings reload",Field::config.channels[0]==1);
    fieldOpen();testTap(tft.width()-30,tft.height()-24);rowTap(2);ck("language opens",FieldUI::currentPage()==8);
    ck("Hebrew hidden initially",!Field::config.hebrew);
    for(int i=0;i<6;i++)Lang::next();ck("normal cycle excludes Hebrew",Field::config.language==0);
    for(int i=0;i<6;i++)testTap(tft.width()/2,24);ck("six taps do not unlock",!Field::config.hebrew);
    testTap(tft.width()/2,24);ck("seventh tap reveals",Field::config.hebrew);
    for(int i=0;i<7;i++){Field::config.language=i;testShot(i==0?"language-english":i==1?"language-spanish":i==2?"language-french":i==3?"language-german":i==4?"language-japanese":i==5?"language-chinese":"language-hebrew");}
    Field::save();Field::config.language=0;Field::begin();ck("Hebrew persists when selected",Field::config.language==6&&Field::config.hebrew);
    FieldUI::openHelp();testShot("hebrew-help");
    Field::config.language=4;FieldUI::openHelp();testShot("japanese-help");
    Field::config.language=5;FieldUI::openHelp();testShot("chinese-help");
    Field::config.language=1;state=AppState::SETTINGS;uiSettingsInit(tft);testShot("spanish-settings");
    Field::config.language=0;fieldOpen();testTap(tft.width()-30,tft.height()-24);rowTap(1);rowTap(0);rowTap(1);rowTap(3);
    ck("contrast, motion and controls switch",Field::config.contrast&&Field::config.reduced&&Field::config.large);
    ck("large shared navigation",Theme::computeButtonBar(tft.width(),tft.height()).h>=34);
    rowTap(2);ck("left handed toggles",Field::config.left);testShot("accessibility");
    testTap(tft.width()-30,tft.height()-24);ck("mirrored back works",FieldUI::currentPage()==0);
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
    ck("direct language entry",menuTap(SettingsRow::LANGUAGE)&&FieldUI::currentPage()==8);
    testTap(20,tft.height()-24);ck("language back returns to settings",state==AppState::SETTINGS);
    ck("direct accessibility entry",menuTap(SettingsRow::ACCESSIBILITY)&&FieldUI::currentPage()==7);
    testTap(20,tft.height()-24);ck("accessibility back returns to settings",state==AppState::SETTINGS);
    ck("alerts category opens",menuTap(SettingsRow::ALERTS)&&uiSettingsCurrentPage()==SettingsPage::ALERTS);
    testShot("menu-alerts");
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
    ck("shutdown reachable",reachable(SettingsPage::MAIN,SettingsRow::POWER_CONTROL));
    ck("storage reachable",reachable(SettingsPage::MAIN,SettingsRow::SD_STATUS));
    ck("security reachable",reachable(SettingsPage::MAIN,SettingsRow::SECURITY));
    ck("ignored devices reachable",reachable(SettingsPage::ALERTS,SettingsRow::IGNORED_DEVICES));
    ck("status light reachable",reachable(SettingsPage::APPEARANCE,SettingsRow::STATUS_LIGHT));
    ck("reset reachable",reachable(SettingsPage::SYSTEM,SettingsRow::RESET_STATS));
    ck("clock settings reachable",reachable(SettingsPage::DESK,SettingsRow::TIME_ZONE));
    ck("boring mode reachable",reachable(SettingsPage::FUN,SettingsRow::BORING_MODE));
    uiSettingsInit(tft);uiSettingsOpenPage(SettingsPage::FUN);testStep();
    ck("game section folds",uiSettingsTapHeader(tft,20,35,tft.width(),tft.height()));
    uiSettingsOpenPage(SettingsPage::MAIN);testStep();
    ck("folding subpage leaves main games entry visible",menuTap(SettingsRow::FUN)&&uiSettingsCurrentPage()==SettingsPage::FUN);
    uiSettingsInit(tft);testStep();
    ck("games category opens",menuTap(SettingsRow::FUN)&&uiSettingsCurrentPage()==SettingsPage::FUN);
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

    suite("Care tools, demos, favorites and lock");
    state=AppState::SETTINGS;uiSettingsInit(tft);testStep(400);
    ck("help entry reachable",menuTap(SettingsRow::CARE)&&state==AppState::CARE);
    testShot("care-home");
    {const bool wasRotating=TFT_eSPI::rotates;TFT_eSPI::rotates=true;int oldW=tft.width();
     testTap(tft.width()-14,10);ck("care rotation button works",tft.width()!=oldW);
     for(int i=0;i<3;i++)testTap(tft.width()-14,10);
     ck("care rotation returns",tft.width()==oldW);TFT_eSPI::rotates=wasRotating;}

    testShot("care-home");
    testTap(80,40+((tft.height()-90)/4)+8);
    ck("practice opens",CareUI::page()==CareUI::Page::DEMO);
    const auto beforeDemo=engine.lifetimeTotal();const auto logsBefore=engine.logCount();const auto researchBefore=Research::stats().observed;
    for(int i=0;i<4;i++){testShot("care-demo");testTap(tft.width()-30,tft.height()-24);testTap(70,65);}
    ck("demo cannot alter real history, progression or research",beforeDemo==engine.lifetimeTotal()&&logsBefore==engine.logCount()&&researchBefore==Research::stats().observed);
    CareUI::open(CareUI::Page::FAVORITES);testShot("care-favorites");
    testTap(tft.width()-30,tft.height()-24);auto oldFavorite=Care::favorite(0);
    testTap(70,48);ck("favorite editor changes destination",Care::favorite(0)!=oldFavorite);
    testTap(tft.width()-30,tft.height()-24);testTap(70,48);ck("favorite launches same Breakout route",state==AppState::BREAKOUT);
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
    return report();
}
