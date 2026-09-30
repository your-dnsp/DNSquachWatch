#include <initializer_list>
#include "test_util.h"
#include "settings.h"
#include "theme.h"
#include "clock.h"
#include <Preferences.h>
#include <cstdlib>
#include <sys/stat.h>
namespace Theme { void applyPalette(uint8_t) {} }
namespace Clock { uint8_t zoneCount(){return 1;} const char* zoneName(uint8_t){return "UTC";} void applyZone(uint8_t){} uint8_t zoneStep(uint8_t,int){return 0;} }
int main() {
    mkdir("out",0755);mkdir("out/dnspnvs",0755);
    setenv("SQUACHSIM_NVS","out/dnspnvs",1);remove("out/dnspnvs/settings.nvs");
    suite("Alert duration migration and persistence");
    Settings::load();ck("existing installs default to thirty seconds",Settings::alertSeconds()==30);
    ck("fresh install locks landscape rotation",Settings::rotationLocked()&&Settings::rotation()==3);
    ck("only iBeacon and Tile default off",!Settings::typeEnabled(DetectionType::IBEACON)&&!Settings::typeEnabled(DetectionType::TILE)&&Settings::enabledTypeCount()==uint8_t(DetectionType::COUNT)-3);
    Settings::toggleType(DetectionType::TILE);Settings::load();ck("explicit Tile opt-in survives reload",Settings::typeEnabled(DetectionType::TILE));
    Settings::saveRotation(1);Settings::toggleRotationLock();Settings::markColorChecked();Settings::load();
    ck("explicit orientation survives upgrade/reload",Settings::rotation()==1&&!Settings::rotationLocked());
    Settings::prepareGiftDisplay();Settings::load();
    ck("gift setup repeats color config and locked landscape",!Settings::colorChecked()&&Settings::rotation()==3&&Settings::rotationLocked());
    for(unsigned expected : {45,60,15,30}) {
        Settings::cycleAlertSeconds();Settings::load();
        ck("chosen duration survives reload",Settings::alertSeconds()==expected);
    }
    Preferences p;p.begin("settings",false);p.putUChar("alertsecs",255);p.end();Settings::load();
    ck("invalid stored duration defaults to thirty",Settings::alertSeconds()==30);
    remove("out/dnspnvs/settings.nvs");Settings::load();
    ck("remote update defaults off",!Settings::remoteUpdate());ck("boot update check defaults off",!Settings::updateCheck());
    Settings::toggleRemoteUpdate();Settings::toggleUpdateCheck();Settings::load();
    ck("saved update choices preserved",Settings::remoteUpdate()&&Settings::updateCheck());
    ck("display defaults to normal in generic build",Settings::displayMhz()==40);
    Settings::cycleDisplayMhz();Settings::load();ck("80 MHz survives reload",Settings::displayMhz()==80);
    Settings::cycleDisplayMhz();Settings::load();ck("40 MHz survives reload",Settings::displayMhz()==40);
    p.begin("settings",false);p.putUChar("dispMHz",60);p.end();Settings::load();
    ck("invalid display speed falls back to 40",Settings::displayMhz()==40);
    return report();
}
