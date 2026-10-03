#include "research_submission.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <SD.h>
#include <filesystem>
int main(){
    UserLabels::Target t{};UserLabels::Label l{};t.ble=false;
    const uint8_t mac[]={0xDA,0x27,0xC1,0x44,0x55,0x66};memcpy(t.mac,mac,6);
    strcpy(t.name,"owner\"s\\device");strcpy(l.subtag,"glasses\n\"research");
    char out[1792];assert(ResearchSubmission::format(out,sizeof out,t,l,123,false));
    assert(strstr(out,"DA:27:C1:XX:XX:XX"));assert(!strstr(out,"44:55:66"));assert(!strstr(out,"owner"));assert(strstr(out,"\\u000a\\\"research"));assert(strstr(out,"unverified"));assert(strstr(out,"locally administered"));
    assert(ResearchSubmission::format(out,sizeof out,t,l,123,true));assert(strstr(out,"DA:27:C1:44:55:66"));assert(strstr(out,"owner\\\"s\\\\device"));
    t.ble=true;assert(ResearchSubmission::format(out,sizeof out,t,l,0,false));assert(strstr(out,"public/random type unknown"));
    assert(!ResearchSubmission::format(out,10,t,l,0,false));assert(!out[0]);
    memset(t.name,'x',sizeof t.name);memset(l.subtag,'"',sizeof l.subtag);assert(ResearchSubmission::format(out,sizeof out,t,l,0,true));
    assert(!ResearchSubmission::format(nullptr,10,t,l,0,false));
    TestSD::root="out/research-submission-sd";std::filesystem::remove_all(TestSD::root);std::filesystem::create_directories(TestSD::root);
    char path[88],old[88];assert(ResearchSubmission::save(t,l,0,false,path,sizeof path));strcpy(old,path);assert(SD.exists(old));
    assert(ResearchSubmission::save(t,l,0,false,path,sizeof path));assert(strcmp(old,path)!=0);assert(SD.exists(old));
    TestSD::writesLeft=0;assert(!ResearchSubmission::save(t,l,0,true,path,sizeof path));assert(!path[0]);assert(!SD.exists("/Research Submissions/PRIVATE/report-00000000-0001-PRIVATE.txt"));assert(!SD.exists("/Research Submissions/PRIVATE/report-00000000-0001-PRIVATE.txt.pending"));
    TestSD::writesLeft=-1;assert(ResearchSubmission::save(t,l,0,false,path,sizeof path));assert(strstr(path,"0003"));assert(TestSD::largestRead<=96);
    strcpy(t.name,"\xFF");assert(ResearchSubmission::format(out,sizeof out,t,l,0,true));assert(strstr(out,"\\u00ff"));
    strcpy(l.subtag,"DA:27:C1:44:55:66");assert(ResearchSubmission::format(out,sizeof out,t,l,0,false));assert(!strstr(out,"44:55:66"));
    assert(ResearchSubmission::savePair(t,l,555,path,sizeof path));assert(strstr(path,"REDACTED"));assert(SD.exists("/Research Submissions/PRIVATE/report-0000022b-0001-PRIVATE.txt"));
    UserLabels::storageWipe();assert(!SD.exists("/Research Submissions"));
    std::filesystem::remove_all(TestSD::root);
    puts("Research submission: redaction, consent, JSON escaping, bounds, unverified scope PASS");
}
