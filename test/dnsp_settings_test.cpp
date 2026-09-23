#include <initializer_list>
#include "test_util.h"
#include "settings.h"
#include "theme.h"
#include "clock.h"
#include <Preferences.h>
#include <cstdlib>
#include <sys/stat.h>
namespace Theme { void applyPalette(uint8_t) {} }
namespace Clock { uint8_t zoneCount(){return 1;} const char* zoneName(uint8_t){return "UTC";} void applyZone(uint8_t){} }
int main() {
    mkdir("out",0755);mkdir("out/dnspnvs",0755);
    setenv("SQUACHSIM_NVS","out/dnspnvs",1);remove("out/dnspnvs/settings.nvs");
    suite("Alert duration migration and persistence");
    Settings::load();ck("existing installs default to thirty seconds",Settings::alertSeconds()==30);
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
    return report();
}
