#include "test_util.h"
#include "detection.h"
#include "detection_record.h"
#include "detection_info.h"
#include "simulation.h"
#include "signatures.h"
#include <cstring>

static Detection sample(unsigned id, DetectionType type=DetectionType::FLOCK) {
    Detection d{}; d.mac[0]=0xAC; d.mac[5]=id; d.type=type;
    d.active=true; d.hits=1; d.rssi=-70; d.conf=Confidence::LOW_CONF;
    d.evidence=MatchEvidence::OUI; d.firstSeen=d.lastSeen=1000;
    return d;
}
int main() {
    SimClock::virtualTime=true; SimClock::nowMs=1000;
    suite("Snapshot FIFO, overflow, coalescing and stale events");
    AlertQueue q; Detection out{};
    for (unsigned i=0;i<10;++i) q.push(sample(i));
    ck("bounded burst counts omissions", q.dropped()==2);
    auto newer=sample(3); newer.rssi=-42; q.push(newer);
    bool fifo=true;
    for(unsigned i=0;i<8;++i) { fifo &= q.pop(out,1000) && out.mac[5]==i; if(i==3) fifo &= out.rssi==-42; }
    ck("FIFO retains originals and coalesces pending updates",fifo && !q.pop(out,1000));
    auto d=sample(42);q.push(d);d.mac[5]=9;
    ck("queued snapshot survives source mutation",q.pop(out,1000) && out.mac[5]==42);
    d=sample(1);d.lastSeen=0xFFFFFFF0u;q.push(d);
    ck("uptime wraparound does not expire recent events",q.pop(out,20));
    q.push(sample(2));ck("two-minute-old events expire",!q.pop(out,122000));
    for(unsigned i=0;i<8;++i)q.push(sample(i));
    q.pop(out,1000);q.pop(out,1000);q.push(sample(8));q.push(sample(9));
    const auto kept=q.retain([](const Detection& x){return x.mac[5]%2==0;},1000);
    fifo=kept==4; for(unsigned i=2;i<10;i+=2) fifo &= q.pop(out,1000) && out.mac[5]==i;
    ck("filtering a wrapped queue keeps order",fifo && !q.pop(out,1000));
    q.push(sample(3));q.clear();ck("clear removes all pending events",!q.pop(out,1000));

    suite("Encounter counts and evidence upgrades use production merger");
    auto row=sample(1); auto seen=row; seen.rssi=-55;seen.channel=6;
    for(unsigned i=0;i<100;++i) mergeObservation(row,seen,1000+i);
    ck("WiFi packets update RSSI without inflating encounters",row.hits==1 && row.channel==6 && row.rssi==-55);
    seen.channel=0;mergeObservation(row,seen,1200);
    ck("Bluetooth has identical encounter semantics",row.hits==1 && row.channel==0);
    row.active=false;row.restored=1;
    ck("returning device raises one new alert",mergeObservation(row,seen,1300) && row.hits==2 && row.active && !row.restored);
    seen.conf=Confidence::HIGH_CONF;seen.evidence=MatchEvidence::BLE_SERVICE;seen.signature=0xFD5F;
    ck("stronger evidence upgrades while still present",mergeObservation(row,seen,1400) && row.conf==Confidence::HIGH_CONF && row.signature==0xFD5F && row.hits==2);
    seen.conf=Confidence::LOW_CONF;seen.evidence=MatchEvidence::OUI;
    ck("weaker packets cannot erase stronger evidence",!mergeObservation(row,seen,1500) && row.conf==Confidence::HIGH_CONF && row.evidence==MatchEvidence::BLE_SERVICE);
    row.active=false;row.hits=UINT16_MAX;mergeObservation(row,seen,1600);
    ck("encounter count saturates instead of wrapping",row.hits==UINT16_MAX);
    memset(seen.name,'X',sizeof seen.name);mergeObservation(row,seen,1700);
    ck("unterminated radio name stays bounded",row.name[sizeof row.name-1]==0);

    suite("Production history ring accounting");
    DetectionEngine eng;
    for(unsigned i=0;i<80;++i)eng.postBle(sample(i));
    ck("eviction bounds both history and live totals",eng.logCount()==64 && eng.countByType(DetectionType::FLOCK)==64 && eng.lifetimeTotal()==80);
    for(unsigned i=0;i<64;++i)eng.postBle(sample(i,DetectionType::AIRTAG));
    ck("replacing other types removes their live counts",eng.countByType(DetectionType::FLOCK)==0 && eng.countByType(DetectionType::AIRTAG)==64);
    eng.resetLifetime();ck("reset lifetime preserves live observations",eng.lifetimeTotal()==0 && eng.countByType(DetectionType::AIRTAG)==64);
    d=sample(1);d.type=(DetectionType)255;eng.postBle(d);
    ck("invalid type cannot index counters",eng.lifetimeTotal()==0 && eng.logCount()==64);
    eng.clearLog();ck("history clear clears alert queue",eng.logCount()==0 && !eng.alerts.pop(out,1000));

    suite("Evidence is explanatory, never invented certainty");
    d=sample(1);auto why=DetectionInfo::why(d);
    ck("shared prefix explicitly does not confirm",strstr(why,"does not confirm") && strstr(why,"AC:00:00"));
    d.evidence=MatchEvidence::UNKNOWN;
    ck("legacy history does not invent evidence",strstr(DetectionInfo::why(d),"not saved"));
    d.evidence=MatchEvidence::BLE_SERVICE;d.signature=0xFD5F;
    ck("exact service ID accompanies explanation",strstr(DetectionInfo::why(d),"FD5F"));
    d.evidence=MatchEvidence::DEAUTH_BURST;d.hits=6;d.firstSeen=0xFFFFFF00u;d.lastSeen=0x000000F4u;
    d.evidenceBits=3|DEAUTH_META_REASON_VALID|DEAUTH_META_UNPROTECTED_SEEN|DEAUTH_META_SAME_BSSID|DEAUTH_META_BSSID_VALID;
    d.signature=7;strcpy(d.name,"06:00:00:00:00:09");why=DetectionInfo::why(d);
    ck("deauth evidence names count, targets, BSSID and spoofing limit",strstr(why,"6 deauth")&&strstr(why,"3 targets")&&strstr(why,"reason 7")&&strstr(why,"can be spoofed"));
    ck("deauth duration remains correct across millis rollover",strstr(why,"500ms"));
    ck("RSSI is not presented as distance",strstr(DetectionInfo::rssiConfidencePrimer(),"not distance"));
    suite("DNSP decoded-address convention and preservation");
    uint8_t exact[]={0xAC,0x12,0x34,0,0,0};
    ck("exact zero suffix matches independently of OUI",Simulation::marked(exact));
    bool misses=true;for(unsigned i=3;i<6;i++){exact[i]=1;misses&=!Simulation::marked(exact);exact[i]=0;}
    ck("each near miss stays ordinary",misses);
    uint8_t vendorMac[]={0xB4,0x1E,0x52,1,2,3},testMac[]={0xB4,0x1E,0x52,0,0,0};Confidence aConf,bConf;
    auto aType=lookupOui(vendorMac,&aConf),bType=lookupOui(testMac,&bConf);
    ck("simulation suffix cannot change OUI classification or confidence",aType==bType&&aConf==bConf);
    uint8_t unknown[]={0x00,0x00,0x01,0,0,0};ck("marker alone cannot create an OUI match",Simulation::marked(unknown)&&lookupOui(unknown)==DetectionType::UNKNOWN);

    uint8_t payload[]={0xAC,0x12,0x34,1,2,3,0,0,0};
    ck("unrelated payload zeros never mark the decoded address",!Simulation::marked(payload));
    char sub[48];Simulation::subtags(sub,sizeof sub,"friend",exact);
    ck("automatic subtag preserves user text",!strcmp(sub,"friend / SIMULATED"));
    Simulation::subtags(sub,sizeof sub,"friend / SIMULATED",exact);
    ck("automatic subtag is not duplicated",!strcmp(sub,"friend / SIMULATED"));
    Simulation::subtags(sub,sizeof sub,"friend",payload);ck("ordinary subtag unchanged",!strcmp(sub,"friend"));
    row=sample(0);row.addressRole=AddressRole::BSSID;seen=row;seen.conf=Confidence::HIGH_CONF;seen.signature=42;
    mergeObservation(row,seen,1800);q.clear();q.push(row);q.pop(out,1800);
    ck("merge and queued snapshot keep classification and provenance",Simulation::marked(out)&&out.type==DetectionType::FLOCK&&out.conf==Confidence::HIGH_CONF&&out.signature==42&&out.addressRole==AddressRole::BSSID);
    seen=row;seen.conf=Confidence::LOW_CONF;seen.evidence=MatchEvidence::OUI;seen.addressRole=AddressRole::TRANSMITTER;mergeObservation(row,seen,1900);
    ck("weaker observation cannot replace retained evidence provenance",row.addressRole==AddressRole::BSSID);
    ck("details explain convention without claiming authenticated identity",strstr(DetectionInfo::why(out),"Real hardware")&&strstr(DetectionInfo::why(out),"BSSID"));
    return report();
}
