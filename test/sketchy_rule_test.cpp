#include "sketchy_rule.h"
#include "test_util.h"
#include <cstring>

static Detection make(DetectionType type,uint8_t id,uint32_t at){Detection d{};d.type=type;d.mac[0]=0x10;d.mac[5]=id;d.rssi=-55;d.channel=6;d.hits=type==DetectionType::DEAUTH?18:1;d.firstSeen=d.lastSeen=at;d.active=true;return d;}

int main(){
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
    return report();
}
