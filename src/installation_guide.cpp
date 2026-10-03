#include "installation_guide.h"
#if defined(ARDUINO_ARCH_ESP32) || defined(INSTALLATION_GUIDE_TEST)
#include <SD.h>
#endif
namespace InstallationGuide {
size_t size(){return 10405;}
bool write(WriteFn writer,void* context){
#if defined(ARDUINO_ARCH_ESP32) || defined(INSTALLATION_GUIDE_TEST)
 if(!writer)return false;File f=SD.open("/DNSP Content/v1.5/DNSQUACHWATCH INSTALLATION.txt",FILE_READ);
 if(!f||f.size()!=size()){f.close();return false;}
 uint8_t block[128];uint32_t h=2166136261u;size_t left=size();
 // Validate before emitting so incomplete or modified content never passes as
 // the installation guide. Re-read uses only one card handle plus backup output.
 while(left){size_t want=left>sizeof block?sizeof block:left;int n=f.read(block,want);if(n!=(int)want){f.close();return false;}for(int i=0;i<n;i++)h=(h^block[i])*16777619u;left-=want;}
 if(h!=0x6c9b6d0fu||!f.seek(0)){f.close();return false;}
 left=size();while(left){size_t want=left>sizeof block?sizeof block:left;int n=f.read(block,want);if(n!=(int)want||!writer(context,block,want)){f.close();return false;}left-=want;}f.close();return true;
#else
 (void)writer;(void)context;return false;
#endif
}
}
