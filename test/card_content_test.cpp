#include "card_content.h"
#include <SD.h>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unistd.h>
int main(){TestSD::root="../microSD-content";TestSD::maxHandles=2;CardContent::Guide view{};
 for(unsigned i=0;i<CardContent::guideCount;i++){char path[80];snprintf(path,sizeof path,"../microSD-content/DNSP Content/v1.5/guide-%03u.txt",i);std::ifstream f(path);std::string title,body;getline(f,title);getline(f,body);CardContent::guide(i,true,view);assert(title==view.title&&body==view.body);}
 CardContent::reset();CardContent::guide(0,false,view);assert(strstr(view.title,"CONTENT NEEDED"));
 char temp[]="/tmp/dnsp-card-content-XXXXXX";assert(mkdtemp(temp));TestSD::root=temp;std::filesystem::create_directories(std::string(temp)+"/DNSP Content/v1.5");std::ofstream f(std::string(temp)+"/DNSP Content/v1.5/guide-000.txt");f<<"tampered\nwrong";f.close();CardContent::reset();CardContent::guide(0,true,view);assert(strstr(view.title,"CONTENT NEEDED"));assert(TestSD::peakHandles<=2);std::filesystem::remove_all(temp);puts("All card pages exact; missing/corrupt content safe fallback PASS");}
