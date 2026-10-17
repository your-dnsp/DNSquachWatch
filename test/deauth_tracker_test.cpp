#include "test_util.h"
#include "deauth_tracker.h"
#include "simulation.h"
#include <cstring>
#include <vector>
#include <random>

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
                      uint8_t count=12, uint8_t targetBase=1) {
    unsigned alerts=0;
    for(uint8_t i=0;i<count;i++) alerts += t.note(frame(source,targetBase+i),start+100u*i).alert;
    return alerts;
}

int main() {
    suite("Per-source deauthentication bursts");
    DeauthBurstTracker five;
    ck("eleven unicast frames do not alert",burst(five,1,1000,11)==0);
    auto sixth=five.note(frame(1,12),2100);
    ck("twelve from one claimed transmitter alert once",sixth.alert&&sixth.count==12&&sixth.source[5]==1);

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
    alerts=0;for(uint8_t i=0;i<13;i++)alerts+=sliding.note(frame(2),300u*i).alert;
    ck("true window rejects twelve whose total span exceeds three seconds",alerts==0);

    DeauthBurstTracker targets;
    DeauthBurstResult one{};
    for(uint8_t i=0;i<12;i++)one=targets.note(frame(3,1,9,i==11),10000+100*i);
    ck("one repeated receiver reports one target",one.alert&&one.distinctTargets==1);
    ck("protected and unprotected observations are retained",one.protectedSeen&&one.unprotectedSeen);
    DeauthBurstTracker many;
    DeauthBurstResult multi{};
    for(uint8_t i=0;i<12;i++)multi=many.note(frame(4,i+1,9),12000+100*i);
    ck("several receivers report several bounded targets",multi.alert&&multi.distinctTargets==12&&multi.sameBssid);
    DeauthBurstTracker mixed;
    DeauthBurstResult b{};
    for(uint8_t i=0;i<12;i++)b=mixed.note(frame(5,1,i<3?9:10),14000+100*i);
    ck("mixed BSSIDs are not reported as one network",b.alert&&!b.sameBssid);

    DeauthBurstTracker rollover;
    DeauthBurstResult wrapped{};
    const uint32_t nearWrap=0xFFFFFF00u;
    for(uint8_t i=0;i<12;i++)wrapped=rollover.note(frame(6,i+1),nearWrap+100u*i);
    ck("millis rollover keeps a valid burst",wrapped.alert&&wrapped.lastMs-wrapped.firstMs==1100);

    suite("Frame bounds and fixed-table pressure");
    uint8_t raw[26]{};raw[0]=0xC0;raw[1]=0x40;
    for(uint8_t i=0;i<6;i++){raw[4+i]=i;raw[10+i]=10+i;raw[16+i]=20+i;}
    raw[24]=7;raw[25]=0;
    DeauthFrameEvidence parsed{};
    ck("short management frame is rejected",!parseDeauthFrame(raw,23,-40,11,parsed));
    ck("24-byte frame parses addresses without reading reason",parseDeauthFrame(raw,24,-40,11,parsed)&&!parsed.reasonValid&&parsed.source[0]==10);
    ck("protected body is not misread as plaintext reason",parseDeauthFrame(raw,26,-40,11,parsed)&&!parsed.reasonValid&&parsed.protectedFrame);
    raw[1]=0;ck("unprotected reason parses when present",parseDeauthFrame(raw,26,-40,11,parsed)&&parsed.reasonValid&&parsed.reason==7&&!parsed.protectedFrame);
    raw[0]=0xA0;ck("disassociation subtype uses the same bounded parser",parseDeauthFrame(raw,26,-40,11,parsed)&&parsed.reasonValid);
    raw[0]=0x80;ck("non-deauth subtype is rejected",!parseDeauthFrame(raw,26,-40,11,parsed));

    DeauthBurstTracker pressure;
    for(uint8_t i=0;i<5;i++)pressure.note(frame(1),100u*i);
    for(uint8_t s=2;s<=13;s++)pressure.note(frame(s),1000u+s);
    ck("oldest entry is evicted deterministically at capacity",!pressure.note(frame(1),1200).alert);
    ck("tracker has a fixed modest RAM bound",DeauthBurstTracker::TABLE_CAP==8&&DeauthBurstTracker::tableBytes()<=1344);
    suite("Simulation follows the claimed transmitter only");
    DeauthBurstTracker ordinarySource, simulatedSource;DeauthBurstResult a{},s{};
    auto ordinary=frame(1);ordinary.destination[3]=ordinary.destination[4]=ordinary.destination[5]=0xDD;ordinary.bssid[3]=ordinary.bssid[4]=ordinary.bssid[5]=0xDD;
    auto marked=frame(0);marked.source[3]=marked.source[4]=marked.source[5]=0xDD;
    for(unsigned i=0;i<12;i++){a=ordinarySource.note(ordinary,30000+100*i);s=simulatedSource.note(marked,30000+100*i);}
    ck("DD-suffix receiver and BSSID cannot mark ordinary source",a.alert&&!Simulation::marked(a.source));
    ck("marked transmitter still requires full ordinary twelve-frame burst",s.alert&&s.count==12&&Simulation::marked(s.source));
    ck("marked source retains ordinary cooldown",!simulatedSource.note(marked,31200).alert);
    suite("Broadcast threshold and exact sliding-window boundaries");
    auto bc=frame(42);memset(bc.destination,0xFF,6);
    DeauthBurstTracker broadcast;DeauthBurstResult br{};
    for(unsigned i=0;i<5;i++)ck("five broadcasts do not alert",!broadcast.note(bc,1000+100*i).alert);
    br=broadcast.note(bc,1500);ck("six broadcasts from one source alert",br.alert&&br.count==6&&br.distinctTargets==1);
    ck("broadcast source cooldown is retained",!broadcast.note(bc,1600).alert);
    DeauthBurstTracker multicast;auto mc=bc;mc.destination[0]=0x01;
    for(unsigned i=0;i<6;i++)ck("multicast is not the all-FF broadcast address",!multicast.note(mc,1000+100*i).alert);
    DeauthBurstTracker splitBc;
    for(unsigned i=0;i<6;i++){auto f=bc;f.source[5]=i;ck("broadcast frames cannot combine across sources",!splitBc.note(f,1000+100*i).alert);}
    DeauthBurstTracker boundary,expired;
    for(unsigned i=0;i<5;i++){boundary.note(bc,1000+i*500);expired.note(bc,1000+i*500);}
    ck("3000ms boundary is included",boundary.note(bc,4000).alert);
    ck("3001ms-old broadcast frame expires",!expired.note(bc,4001).alert);
    DeauthBurstTracker mixedTraffic;DeauthBurstResult mx{};
    for(unsigned i=0;i<11;i++){auto f=frame(42);if(i%2==0)memset(f.destination,0xFF,6);mx=mixedTraffic.note(f,1000+100*i);if(i<10)ck("mixed traffic waits for sixth broadcast",!mx.alert);}
    ck("six broadcast frames with interleaved unicast alert",mx.alert&&mx.count==11);
    ck("cooldown blocks before 15000ms",!broadcast.note(bc,16499).alert);
    for(unsigned i=0;i<4;i++)broadcast.note(bc,16500+i);
    ck("cooldown expires at 15000ms with a fresh six-frame window",broadcast.note(bc,16504).alert);
    DeauthBurstTracker networkChange;DeauthBurstResult net{};
    for(unsigned i=0;i<12;i++)net=networkChange.note(frame(7,1,9),1000+100*i);
    ck("same BSSID is exact before network change",net.alert&&net.sameBssid&&net.bssid[5]==9);
    networkChange.note(frame(7,1,10),17000);
    for(unsigned i=0;i<11;i++)net=networkChange.note(frame(7,1,11),17010+100*i);
    ck("network switch within window is mixed",net.alert&&!net.sameBssid);
    for(unsigned i=0;i<12;i++)net=networkChange.note(frame(7,1,11),34000+100*i);
    ck("expired previous BSSID does not contaminate new burst",net.alert&&net.sameBssid&&net.bssid[5]==11);
    DeauthBurstTracker simultaneous;DeauthBurstResult tie{};
    for(unsigned i=0;i<12;i++)tie=simultaneous.note(frame(8,1,i<6?9:10),50000);
    ck("BSSID changes remain mixed with identical queued timestamps",tie.alert&&!tie.sameBssid);
    suite("Independent sliding-window oracle, including rollover and timestamp ties");
    struct Sample { DeauthFrameEvidence frame; uint32_t at; };
    std::vector<Sample> history;std::mt19937 random(139192);DeauthBurstTracker stress;
    uint32_t now=0xfffff000u,lastFire=0;bool fired=false,valid=true;
    for(unsigned step=0;step<30000&&valid;++step){
        now+=step%23==0?4001u:random()%500;
        auto f=frame(9,random()%16,9+random()%3,random()%2,random()%65536);f.reasonValid=!f.protectedFrame;
        if(random()%3==0)memset(f.destination,0xFF,6);
        history.push_back({f,now});if(history.size()>12)history.erase(history.begin());
        std::vector<Sample> recent;unsigned bcCount=0;
        for(auto h:history)if(uint32_t(now-h.at)<=3000){recent.push_back(h);bool bc=true;for(auto x:h.frame.destination)bc&=x==0xFF;bcCount+=bc;}
        bool expected=(recent.size()>=12||bcCount>=6)&&(!fired||uint32_t(now-lastFire)>=15000);
        auto actual=stress.note(f,now);valid=actual.alert==expected;
        if(expected){
            bool same=true,known=false;uint16_t reason=0;
            for(auto h:recent){same&=!memcmp(h.frame.bssid,f.bssid,6);if(h.frame.reasonValid){known=true;reason=h.frame.reason;}}
            valid&=actual.count==recent.size()&&actual.sameBssid==same&&actual.reasonValid==known&&(!known||actual.reason==reason);
            fired=true;lastFire=now;
        }
    }
    ck("30000 observed frames agree with independent oracle",valid);
    DeauthBurstTracker equalReason;DeauthBurstResult er{};
    for(unsigned i=0;i<20;i++){auto f=frame(10,1,9,false,i);er=equalReason.note(f,1000);}
    for(unsigned i=0;i<11;i++){auto f=frame(10,1,9,false,100+i);equalReason.note(f,16000);}
    er=equalReason.note(frame(10,1,9,false,999),16000);
    ck("latest reason wins after ring wrap with equal timestamps",er.alert&&er.reason==999);
    printf("Tracker RAM: entry=%zu table=%zu queue-evidence=%zu\n",sizeof(DeauthBurstTracker::Entry),DeauthBurstTracker::tableBytes(),sizeof(DeauthFrameEvidence));
    return report();
}
