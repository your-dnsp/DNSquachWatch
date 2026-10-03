#include "sketchy_rule.h"
#include "test_util.h"
#include <cstring>
#include <SD.h>
#include <filesystem>
#include <fstream>
#include <sstream>

static Detection make(DetectionType type,uint8_t id,uint32_t at){Detection d{};d.type=type;d.mac[0]=0x10;d.mac[5]=id;d.rssi=-55;d.channel=6;d.hits=type==DetectionType::DEAUTH?18:1;d.firstSeen=d.lastSeen=at;d.active=true;return d;}

int main(){
    TestSD::root="out/simulation-incident-sd";std::filesystem::remove_all(TestSD::root);std::filesystem::create_directories(TestSD::root);
    SketchyRule::begin();DetectionEngine e;SketchyRule::Incident in{};
    suite("Rolling 90-second rule");
    e.postBle(make(DetectionType::FLOCK,1,1000));SketchyRule::tick(e,1000);
    ck("one side alone does not alert",!SketchyRule::takeAlert(in));
    e.postBle(make(DetectionType::DEAUTH,2,50000));SketchyRule::tick(e,50000);
    ck("Flock followed by deauth alerts",SketchyRule::takeAlert(in));
    ck("the paired evidence is preserved",in.alpr.type==DetectionType::FLOCK&&in.deauth.type==DetectionType::DEAUTH&&in.gapSeconds==49);
    SketchyRule::tick(e,51000);ck("continuing pair is deduplicated",!SketchyRule::takeAlert(in));

    DetectionEngine reverse;
    reverse.postBle(make(DetectionType::DEAUTH,4,200000));SketchyRule::tick(reverse,200000);
    reverse.postBle(make(DetectionType::AXON,3,250000));SketchyRule::tick(reverse,250000);
    ck("order does not matter and Axon qualifies",SketchyRule::takeAlert(in)&&in.alpr.type==DetectionType::AXON);

    DetectionEngine old;
    old.postBle(make(DetectionType::ALPR,5,400000));old.postBle(make(DetectionType::DEAUTH,6,491000));SketchyRule::tick(old,491000);
    ck("more than 90 seconds does not alert",!SketchyRule::takeAlert(in));
    DetectionEngine sim;auto alpr=make(DetectionType::FLOCK,0,600000);alpr.addressRole=AddressRole::BLE_ADVERTISER;
    auto deauth=make(DetectionType::DEAUTH,0,600500);deauth.mac[0]=0x20;deauth.addressRole=AddressRole::TRANSMITTER;
    sim.postBle(alpr);sim.postBle(deauth);SketchyRule::tick(sim,600500);
    ck("both simulated endpoints produce SIMULATED incident",SketchyRule::takeAlert(in)&&!strcmp(SketchyRule::simulationLabel(in),"SIMULATED"));
    SketchyRule::begin();SketchyRule::Incident saved{};
    ck("incident endpoint provenance survives NVS restoration",SketchyRule::recent(0,saved)&&saved.alpr.addressRole==AddressRole::BLE_ADVERTISER&&saved.deauth.addressRole==AddressRole::TRANSMITTER&&!strcmp(SketchyRule::simulationLabel(saved),"SIMULATED"));
    DetectionEngine mixed;alpr.lastSeen=alpr.firstSeen=800000;deauth.mac[5]=7;deauth.lastSeen=deauth.firstSeen=800100;
    mixed.postBle(alpr);mixed.postBle(deauth);SketchyRule::tick(mixed,800100);
    ck("mixed incident identifies the simulated endpoint",SketchyRule::takeAlert(in)&&!strcmp(SketchyRule::simulationLabel(in),"CONTAINS SIMULATED EVIDENCE")&&Simulation::marked(in.alpr.mac)&&!Simulation::marked(in.deauth.mac));
    std::ifstream file(TestSD::root+"/Sketchy Environment/incident-"+std::to_string(in.id)+".txt");std::stringstream text;text<<file.rdbuf();
    ck("incident export retains independent endpoint flags",text.str().find("CONTAINS SIMULATED EVIDENCE")!=std::string::npos&&text.str().find("ALPR simulated: 1")!=std::string::npos&&text.str().find("DEAUTH simulated: 0")!=std::string::npos);
    std::filesystem::remove_all(TestSD::root);
    return report();
}
