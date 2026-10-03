// CYD adaptation of SquachWatch v1.26's charge/runtime tools. All entry paths
// stay behind the existing duress, safe boot and safe shutdown branches.
extern volatile bool g_consoleRuntime;
static void runtimeTick(uint32_t now) {
    static uint32_t last=0; static bool first=true;
    if(first || now-last>=600000UL){first=false;last=now;Settings::noteRunMinutes(BlackBox::bootNumber(),now/60000UL);}
    if(g_consoleRuntime){g_consoleRuntime=false;uint16_t boots[8],mins[8];uint8_t n=Settings::runHistory(boots,mins,8);
        for(uint8_t i=0;i<n;++i)Serial.printf("[runtime] boot %u: %uh %02um\n",boots[i],mins[i]/60,mins[i]%60);}
}
#if defined(ARDUINO_ARCH_ESP32) && !defined(TWATCH_S3)
static bool s_chargeMode=false, s_chargeDown=true;
static uint32_t s_chargeAt=0,s_chargeLitAt=0,s_chargeUntil=0,s_chargeUp=0;
static void chargeBacklight(uint8_t duty){if(s_backlightReady)ledcWrite(BL_CH_ORIG,duty);}
static void chargeDraw(uint32_t now){
    drawTwoBand([&](TFT_eSPI& t,bool){
        t.fillRect(0,0,t.width(),t.height(),Theme::BG);Theme::drawTitleBar(t,"CHARGE MODE");
        char text[256];uint32_t m=(now-s_chargeAt)/60000UL;
        snprintf(text,sizeof text,"Low power mode: %luh %02lum\nRadios and status light are off. Battery charge level is not measured. Tap the dark screen for the timer; tap again to resume. Use Safe Shutdown before removing power or microSD.",(unsigned long)(m/60),(unsigned long)(m%60));
        Lang::draw(t,text,12,40,t.width()-24,t.height()-54,Theme::WHITE);
    });
#if !SQW_BANDED_FRAME
    if(frameBufferOk)pushFrame(0,0);
#endif
}
static void exitChargeMode(){s_chargeMode=false;s_bootDark=false;engine.wakeRadios();applyCpuClock();lastTouch=millis();s_screenDimmed=false;applyBrightness();FramePush::invalidate();if(Security::locked())enterLocked();else enterClear();}
static void requestChargeMode(uint32_t now){
    if(ReadableLogs::automatic())ReadableLogs::cancel(); // journal makes background refresh resumable
    // Never interrupt a storage operation, an update, or an authenticated
    // workflow. REST owns both radios only after the other owners settle.
    if(Security::locked() || state==AppState::PIN_ENTRY || state==AppState::SECURITY || Backup::busy() || ReadableLogs::busy() || Research::active() || !Research::settled() || !DroneWatch::settled() || DroneWatch::focused() || OtaWifi::state()!=OtaWifi::State::OFF || OtaBle::state()!=OtaBle::State::OFF || !engine.restRadios(true)){
        Theme::showToast("CHARGE MODE WAIT", "Finish the active task first",Theme::AMBER);return;
    }
    Field::telemetryStop(); // own-equipment BLE links must rest along with scanning
    s_chargeMode=true;s_chargeAt=s_chargeLitAt=now;s_chargeUntil=now+15000;s_chargeDown=true;s_chargeUp=now;
    StatusLight::off();setCpuFrequencyMhz(80);s_cpuMhzApplied=80;chargeBacklight(Settings::brightness()/2);chargeDraw(now);
}
static bool chargeTick(uint32_t now){
    static bool bootWas=false,longDone=false;static uint32_t bootAt=0;
    const bool boot=digitalRead(0)==LOW;
    if(boot&&!bootWas){bootAt=now;longDone=false;}
    if(boot&&!longDone&&now-bootAt>=1200){longDone=true;if(s_chargeMode)exitChargeMode();else g_consoleCharge=true;}
    if(!boot&&bootWas&&!longDone&&now-bootAt>=30){if(s_chargeMode)exitChargeMode();else{s_bootDark=!s_bootDark;chargeBacklight(s_bootDark?0:s_backlightDuty);lastTouch=now;}}
    bootWas=boot;
    if(g_consoleCharge){g_consoleCharge=false;if(s_chargeMode)exitChargeMode();else requestChargeMode(now);}
    if(!s_chargeMode)return false;
    // Charge mode pauses the normal loop, so enforce its PIN policy here too.
    const bool dark=!s_chargeUntil||(int32_t)(s_chargeUntil-now)<=0;
    const uint32_t lockMs=Security::autoLockIdleMs();
    if(Security::enabled()&&!Security::locked()&&((Security::autoLockOnSleep()&&dark)||(lockMs&&now-s_chargeAt>=lockMs)))Security::lock();
    const TouchPoint p=pollTouch();bool down=p.valid;
    if(!down&&s_chargeDown)s_chargeUp=now;
    if(down&&!s_chargeDown&&now-s_chargeUp>150){
        if(s_chargeUntil&&(int32_t)(s_chargeUntil-now)>0&&now-s_chargeLitAt>800){exitChargeMode();return true;}
        s_chargeLitAt=now;s_chargeUntil=now+5000;chargeBacklight(Settings::brightness()/2);chargeDraw(now);
    }
    s_chargeDown=down;
    if(s_chargeUntil&&(int32_t)(s_chargeUntil-now)<=0){s_chargeUntil=0;chargeBacklight(0);}
    delay(40);return true;
}
#endif
