#pragma once
#include "draw_band.h"
#include "ui_alert.h"
#include "ui_watchalert.h"
#include "ui_colorcheck.h"
#include "ui_sysprops.h"
#include "ui_wifipass.h"
#include "ui_device.h"
#include "ui_settings.h"
#include "ui_boot.h"
#include "fast_sprite.h"
#include "remington.h"
#include "ui_location.h"
#include "ui_labels.h"

static int runBandedUiChecks() {
    suite("Half-screen storage, logical geometry and composed pixels");
    for (int orientation=0;orientation<2;orientation++) {
        const int w=orientation?240:320,h=orientation?320:240;
        TFT_eSPI fullPanel(w,h),bandPanel(w,h);
        TFT_eSprite full(&fullPanel),band(&bandPanel);
        full.setColorDepth(8);band.setColorDepth(8);
        full.createSprite(w,h);band.createSprite(w,h/2);
        auto check=[&](const char* name,auto draw) {
            DrawBand::all();full.resetViewport();draw(full);full.fillRect(0,0,w,h,Theme::BG);
            randomSeed(123);draw(full);full.pushSprite(0,0);
            for(int offset=0;offset<h;offset+=h/2) {
                band.setViewport(0,-offset,w,h,true);
                ck("viewport reports full screen height",band.height()==h);
                ck("viewport never grows the half buffer",band.pixelsRGB565().size()==size_t(w*h/2));
                DrawBand::set(offset,offset+h/2);
                band.fillRect(0,0,w,h,Theme::BG);randomSeed(123);draw(band);band.pushSprite(0,offset);
            }
            DrawBand::all();band.resetViewport();
            const auto& a=fullPanel.pixelsRGB565();const auto& b=bandPanel.pixelsRGB565();
            size_t diff=0;for(size_t i=0;i<a.size();i++)if(a[i]!=b[i])diff++;
            if(diff)printf("%s: %zu differing pixels (%dx%d)\n",name,diff,w,h);
            ck(name,a==b);
        };
        check("boundary-crossing text and shapes",[&](TFT_eSPI& t){
            t.fillRect(5,h/2-10,w-10,20,Theme::CYAN);
            t.drawCircle(w/2,h/2,40,Theme::PINK);
            t.setTextFont(1);t.setTextSize(2);t.setTextColor(Theme::WHITE,Theme::BG);
            t.setCursor(10,h/2-7);t.print("Both halves");
            t.drawLine(0,0,w-1,h-1,Theme::GREEN);
        });
        check("color calibration",[&](TFT_eSPI& t){uiColorCheckTick(t,10000);});
        uiBootInit(full);
        check("splash",[&](TFT_eSPI& t){uiBootTick(t,10000);});
        Detection d{};d.type=DetectionType::FLOCK;d.conf=Confidence::LOW_CONF;d.vendor="Test";d.rssi=-50;d.hits=3;
        uiAlertInit(full,d);uiAlertSetPending(4,0);
        check("alert and Snooze All",[&](TFT_eSPI& t){uiAlertTick(t,10000,engine,false,nullptr,nullptr,false);});
        check("alert explanation overlay",[&](TFT_eSPI& t){uiAlertTick(t,10000,engine,true,"WHY THIS MATCHED","Manufacturer match does not confirm a camera.",false);});
        UserLabels::Target labelTarget{};labelTarget.mac[0]=0xA4;labelTarget.mac[5]=0xEE;
        labelTarget.original=DetectionType::FLOCK;strcpy(labelTarget.name,"Research Device");
        UserLabels::Label label{};label.type=(uint8_t)DetectionType::FLOCK;strcpy(label.subtag,"CAMERA");
        ck("research label restored",UserLabels::restore(labelTarget.mac,label));LabelUI::open(labelTarget);
        check("research choice paints both halves",[&](TFT_eSPI& t){LabelUI::draw(t);});
        LabelUI::tap(80,182,w,h,10000);
        check("research export consent paints both halves",[&](TFT_eSPI& t){LabelUI::draw(t);});
        LabelUI::tap(80,110,w,h,10001);
        check("research identifier opt-in paints both halves",[&](TFT_eSPI& t){LabelUI::draw(t);});
        LabelUI::tap(w-30,h-25,w,h,10002);
        check("research export failure paints both halves",[&](TFT_eSPI& t){LabelUI::draw(t);});LabelUI::close();
        uiSettingsInit(full);
        check("settings",[&](TFT_eSPI& t){uiSettingsTick(t,10000,engine);});
        uiSysPropsInit(full);
        check("system update window",[&](TFT_eSPI& t){uiSysPropsTick(t,10000,engine,false);});
        uiSysPropsShowCredits();
        check("credits window",[&](TFT_eSPI& t){uiSysPropsTick(t,10000,engine,false);});
        const auto creditsTop = fullPanel.pixelsRGB565();
        const int propsX = (w - (w-8<300?w-8:300))/2 + 4;
        const int propsBottom = (h - (h-8<232?h-8:232))/2 + (h-8<232?h-8:232) - 6 - 24;
        uiSysPropsTouch(full, propsX+48, propsBottom+12);
        check("credits Down paints both halves",[&](TFT_eSPI& t){uiSysPropsTick(t,10000,engine,false);});
        ck("credits Down changes content",fullPanel.pixelsRGB565()!=creditsTop);
        uiSysPropsTouch(full, propsX+16, propsBottom+12);
        check("credits Up paints both halves",[&](TFT_eSPI& t){uiSysPropsTick(t,10000,engine,false);});
        ck("credits Up restores content",fullPanel.pixelsRGB565()==creditsTop);
        uiWifiPassInit(full, "Test network");
        check("settled WiFi keyboard survives cleared bands",[&](TFT_eSPI& t){uiWifiPassTick(t,millis()+1000,true);});
        check("idle WiFi keyboard survives another cleared frame",[&](TFT_eSPI& t){uiWifiPassTick(t,millis()+1000,true);});
        check("Remington photo composes both bands",[&](TFT_eSPI& t){Remington::photo(t);});
        check("location page composes both bands",[&](TFT_eSPI& t){LocationUI::draw(t);});
        Remington::shootingStar(full,1000,0,h);Remington::shootingStar(full,37000,0,h);
        check("Remington shooting star composes both bands",[&](TFT_eSPI& t){Remington::shootingStar(t,38000,0,h);});
        DeviceUI::open(DeviceUI::POWER,false,w,h);
        check("read-only help page",[&](TFT_eSPI& t){DeviceUI::draw(t);});
        Theme::showToast("WORKING", "SAVING TO MICROSD", Theme::CYAN);
        check("toast overlay",[&](TFT_eSPI& t){Theme::drawToast(t,millis());});
    }
    DrawBand::all();
    return report();
}
