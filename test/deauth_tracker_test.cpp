#include "test_util.h"
#include "deauth_tracker.h"
#include "simulation.h"
#include <cstring>

static DeauthFrameEvidence frame(uint8_t source, uint8_t target=1, uint8_t bssid=9,
                                 bool protectedFrame=false, uint16_t reason=7) {
    DeauthFrameEvidence f{};
    f.source[0]=0x02; f.source[5]=source;
    f.destination[0]=0x04; f.destination[5]=target;
    f.bssid[0]=0x06; f.bssid[5]=bssid;
    f.rssi=-55; f.channel=6; f.reason=reason; f.reasonValid=true;
    f.protectedFrame=protectedFrame;
    return f;
}

static unsigned burst(DeauthBurstTracker& t, uint8_t source, uint32_t start,
                      uint8_t count=6, uint8_t targetBase=1) {
    unsigned alerts=0;
    for(uint8_t i=0;i<count;i++) alerts += t.note(frame(source,targetBase+i),start+100u*i).alert;
    return alerts;
}

int main() {
    suite("Per-source deauthentication bursts");
    DeauthBurstTracker five;
    ck("five frames do not alert",burst(five,1,1000,5)==0);
    auto sixth=five.note(frame(1,6),1500);
    ck("six from one claimed transmitter alert once",sixth.alert&&sixth.count==6&&sixth.source[5]==1);

    DeauthBurstTracker unrelated;
    unsigned alerts=0;
    for(uint8_t i=0;i<6;i++)alerts+=unrelated.note(frame(i+1),2000+100*i).alert;
    ck("six unrelated transmitters cannot combine",alerts==0);

    DeauthBurstTracker split;
    for(uint8_t i=0;i<3;i++)alerts+=split.note(frame(0xA),3000+100*i).alert;
    for(uint8_t i=0;i<3;i++)alerts+=split.note(frame(0xB),3300+100*i).alert;
    ck("three plus three cannot combine",alerts==0);

    DeauthBurstTracker attributed;
    ck("alert is attributed to source A",burst(attributed,0xA,4000)==1);
    ck("source B alerts during A cooldown",burst(attributed,0xB,4700)==1);
    ck("A cannot repeat during its cooldown",burst(attributed,0xA,5400)==0);
    ck("A can alert again after its cooldown",burst(attributed,0xA,20000)==1);

    suite("Window, evidence, and rollover");
    DeauthBurstTracker gaps;
    for(uint8_t i=0;i<5;i++)gaps.note(frame(1),100u*i);
    ck("expired burst resets instead of accumulating",!gaps.note(frame(1),4001).alert);
    DeauthBurstTracker sliding;
    alerts=0;for(uint8_t i=0;i<7;i++)alerts+=sliding.note(frame(2),700u*i).alert;
    ck("true window rejects six whose total span exceeds three seconds",alerts==0);

    DeauthBurstTracker targets;
    DeauthBurstResult one{};
    for(uint8_t i=0;i<6;i++)one=targets.note(frame(3,1,9,i==5),10000+100*i);
    ck("one repeated receiver reports one target",one.alert&&one.distinctTargets==1);
    ck("protected and unprotected observations are retained",one.protectedSeen&&one.unprotectedSeen);
    DeauthBurstTracker many;
    DeauthBurstResult multi{};
    for(uint8_t i=0;i<6;i++)multi=many.note(frame(4,i+1,9),12000+100*i);
    ck("several receivers report several bounded targets",multi.alert&&multi.distinctTargets==6&&multi.sameBssid);
    DeauthBurstTracker mixed;
    DeauthBurstResult b{};
    for(uint8_t i=0;i<6;i++)b=mixed.note(frame(5,1,i<3?9:10),14000+100*i);
    ck("mixed BSSIDs are not reported as one network",b.alert&&!b.sameBssid);

    DeauthBurstTracker rollover;
    DeauthBurstResult wrapped{};
    const uint32_t nearWrap=0xFFFFFF00u;
    for(uint8_t i=0;i<6;i++)wrapped=rollover.note(frame(6,i+1),nearWrap+100u*i);
    ck("millis rollover keeps a valid burst",wrapped.alert&&wrapped.lastMs-wrapped.firstMs==500);

    suite("Frame bounds and fixed-table pressure");
    uint8_t raw[26]{};raw[0]=0xC0;raw[1]=0x40;
    for(uint8_t i=0;i<6;i++){raw[4+i]=i;raw[10+i]=10+i;raw[16+i]=20+i;}
    raw[24]=7;raw[25]=0;
    DeauthFrameEvidence parsed{};
    ck("short management frame is rejected",!parseDeauthFrame(raw,23,-40,11,parsed));
    ck("24-byte frame parses addresses without reading reason",parseDeauthFrame(raw,24,-40,11,parsed)&&!parsed.reasonValid&&parsed.source[0]==10);
    ck("protected body is not misread as plaintext reason",parseDeauthFrame(raw,26,-40,11,parsed)&&!parsed.reasonValid&&parsed.protectedFrame);
    raw[1]=0;ck("unprotected reason parses when present",parseDeauthFrame(raw,26,-40,11,parsed)&&parsed.reasonValid&&parsed.reason==7&&!parsed.protectedFrame);
    raw[0]=0x80;ck("non-deauth subtype is rejected",!parseDeauthFrame(raw,26,-40,11,parsed));

    DeauthBurstTracker pressure;
    for(uint8_t i=0;i<5;i++)pressure.note(frame(1),100u*i);
    for(uint8_t s=2;s<=13;s++)pressure.note(frame(s),1000u+s);
    ck("oldest entry is evicted deterministically at capacity",!pressure.note(frame(1),1200).alert);
    ck("tracker has a fixed modest RAM bound",DeauthBurstTracker::TABLE_CAP==12&&DeauthBurstTracker::tableBytes()<1600);
    suite("Simulation follows the claimed transmitter only");
    DeauthBurstTracker ordinarySource, simulatedSource;DeauthBurstResult a{},s{};
    for(unsigned i=0;i<6;i++){a=ordinarySource.note(frame(1,0,0),30000+100*i);s=simulatedSource.note(frame(0,1,9),30000+100*i);}
    ck("zero-suffix receiver and BSSID cannot mark ordinary source",a.alert&&!Simulation::marked(a.source));
    ck("marked transmitter still requires full ordinary six-frame burst",s.alert&&s.count==6&&Simulation::marked(s.source));
    ck("marked source retains ordinary cooldown",!simulatedSource.note(frame(0),30600).alert);
    return report();
}
