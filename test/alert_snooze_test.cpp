#include "test_util.h"
#include "alert_snooze.h"
#include "alert_queue.h"
int main(){
 suite("Global popup snooze");
 ck("off on boot",!AlertSnooze::active(0));AlertSnooze::start(0);
 ck("works when clock is zero",AlertSnooze::remaining(0)==600000);
 ck("stays paused before deadline",AlertSnooze::active(599999));
 ck("expires at deadline",!AlertSnooze::active(600000));
 AlertSnooze::start(0xfffffff0u);ck("clock wrap preserves pause",AlertSnooze::remaining(4)==599980);
 AlertSnooze::resume();ck("manual resume immediate",!AlertSnooze::active(5));
 suite("Radio callback timestamp race");
 AlertQueue q;Detection d{};d.type=DetectionType::FLOCK;d.lastSeen=105;q.push(d);
 ck("future-by-one-loop record stays in queue",q.retain([](const Detection&){return true;},100)==1);
 Detection out{};ck("future-by-one-loop record can pop",q.pop(out,100));
 d.lastSeen=10;q.push(d);ck("stale alert still expires",!q.pop(out,120011));
 d.lastSeen=0xfffffff0u;q.push(d);ck("age handles millis wrap",q.pop(out,4));
 return report();
}
