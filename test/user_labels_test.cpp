#include "user_labels.h"
#include "test_util.h"
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

static bool sinkOk = true;
static unsigned sinkCalls = 0;
static bool sink(const UserLabels::Target&, const UserLabels::Label&, uint32_t) {
    sinkCalls++;
    return sinkOk;
}

int main() {
    mkdir("out",0755);mkdir("out/labelnvs",0755);setenv("SQUACHSIM_NVS","out/labelnvs",1);
    remove("out/labelnvs/dnsp-labels.nvs");
    suite("Required export and validation");
    UserLabels::clearSession();
    UserLabels::setExportSink(sink);
    UserLabels::Target target{};
    const uint8_t mac[6] = {0x10,0x20,0x30,0x40,0x50,0x60};
    memcpy(target.mac, mac, 6);
    target.original = DetectionType::AXON;

    UserLabels::Label other{};
    other.type = UserLabels::OTHER_TAG;
    ck("Other requires a subtag", !UserLabels::save(target, other, 1));
    ck("invalid label never reaches export", sinkCalls == 0);

    UserLabels::Label flock{};
    flock.type = (uint8_t)DetectionType::FLOCK;
    sinkOk = false;
    ck("failed microSD export rejects the label", !UserLabels::save(target, flock, 2));
    UserLabels::Label got{};
    ck("failed export does not alter the displayed label", !UserLabels::lookup(mac, got));

    sinkOk = true;
    strcpy(flock.subtag, "GUNSHOT DETECTOR");
    ck("successful export commits the observation", UserLabels::save(target, flock, 3));
    ck("saved label can be looked up", UserLabels::lookup(mac, got));
    ck("type and subtag survive", got.type == flock.type && !strcmp(got.subtag, flock.subtag));

    suite("Editing");
    flock.type = (uint8_t)DetectionType::AXON;
    strcpy(flock.subtag, "CAMERA");
    ck("a later observation replaces the same MAC", UserLabels::save(target, flock, 4));
    ck("edited value is returned", UserLabels::lookup(mac, got) && got.type == flock.type && !strcmp(got.subtag, "CAMERA"));
    suite("Restore replacement");
    UserLabels::clearAll();
    ck("restore can replace all old labels", UserLabels::count()==0 && !UserLabels::lookup(mac,got));
    UserLabels::begin();
    ck("cleared label set stays cleared in storage", UserLabels::count()==0);
    return report();
}
