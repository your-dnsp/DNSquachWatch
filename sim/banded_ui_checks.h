#pragma once
#include "draw_band.h"
#include "ui_alert.h"
#include "ui_watchalert.h"
#include "ui_colorcheck.h"
#include "ui_sysprops.h"
#include "ui_device.h"
#include "ui_settings.h"
#include "ui_boot.h"
#include "fast_sprite.h"

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
        uiSettingsInit(full);
        check("settings",[&](TFT_eSPI& t){uiSettingsTick(t,10000,engine);});
        uiSysPropsInit(full);
        check("system update window",[&](TFT_eSPI& t){uiSysPropsTick(t,10000,engine,false);});
        uiSysPropsShowCredits();
        check("credits window",[&](TFT_eSPI& t){uiSysPropsTick(t,10000,engine,false);});
        DeviceUI::open(DeviceUI::POWER,false,w,h);
        check("read-only help page",[&](TFT_eSPI& t){DeviceUI::draw(t);});
        Theme::showToast("WORKING", "SAVING TO MICROSD", Theme::CYAN);
        check("toast overlay",[&](TFT_eSPI& t){Theme::drawToast(t,millis());});
    }
    DrawBand::all();
    return report();
}
