#include "ui_device.h"
#include "crash_reports.h"
#include "theme.h"
#include "language.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
namespace DeviceUI {
static Page page=HELP;
static bool ready=false,more=false,backup=false;
static int width=320,height=240,cols=24,rows=8,listOffset=0,count=0;
static char names[4][40],lines[14][48],filePath[192],title[48];
static uint32_t offsets[128],nextOffset=0,total=0;
static unsigned index=0;
static const char* body=nullptr;
static const char* general="Reboot the device. Check that a valid microSD is inserted while power is disconnected and provide stable power. Do not unplug while saving. If resets continue, open System > Crash Reports and share a report with DNSP. For detection issues use Alerts & Detection > Troubleshooting.";
static const char* power="Use a basic power brick; 5 V DC, 1A or 2A adapter; like a cheap brick. Use a USB-A (standard USB) to USB-C cable; like the one provided, or a computer USB port, power bank with a USB-A output. USB-C to USB-C cables should be mostly avoided. Avoid supplies that provide 9 V, 12 V, or higher directly. Do not use a laptop charger or other dedicated fast charging USB-C cable.";
static const char* dataYes="Use Storage & Recovery > Backup & Restore to protect data between troubleshooting and firmware updates. A completed DNSP firmware backup was found on this microSD. It can be used to restore that saved version from a computer. It may not be the original shipped version; check its manifest. Copy backups to a computer too. No new integrity check was performed here.";
static const char* dataNo="Use Storage & Recovery > Backup & Restore to protect data between troubleshooting and firmware updates. No completed DNSP firmware backup was found, or the microSD is unavailable. Create a verified backup before updating. Keep the shipped firmware kit on the microSD and a computer so you can restore it over USB. Contact dnsp@duck.com if you need a replacement.";
static bool validName(const char* n){
    if(strlen(n)!=27||strncmp(n,"crash-",6)||n[14]!='-'||strcmp(n+23,".txt"))return false;
    for(int i=6;i<23;++i)if(i!=14&&!isxdigit((unsigned char)n[i]))return false;return true;
}
static void list(){
    count=0;more=false;memset(names,0,sizeof names);
    if(!ready)return;char path[160];snprintf(path,sizeof path,"%s/crash-reports",CrashReports::root());
    DIR* d=opendir(path);if(!d)return;int seen=0;struct dirent* e;
    while((e=readdir(d))){if(!validName(e->d_name))continue;if(seen++<listOffset)continue;
        if(count==4){more=true;break;}snprintf(names[count++],sizeof names[0],"%s",e->d_name);}
    closedir(d);
}
// Cache only one screen. File handles are closed BEFORE either display band draws.
static void load(){
    memset(lines,0,sizeof lines);FILE* f=nullptr;
    uint32_t pos=offsets[index];
    if(!body){struct stat st;if(!ready||stat(filePath,&st)||!S_ISREG(st.st_mode)){strcpy(lines[0],"Report unavailable.");more=false;return;}
        total=st.st_size>16384?16384:st.st_size;f=fopen(filePath,"rb");if(!f){strcpy(lines[0],"Cannot read report.");more=false;return;}fseek(f,pos,SEEK_SET);
    }else total=strlen(body);
    for(int row=0;row<rows&&pos<total;++row){
        char buf[48]={};unsigned n=0;int space=-1;uint32_t start=pos;
        while(n<unsigned(cols)&&pos<total){int c=body?(unsigned char)body[pos]:fgetc(f);if(c==EOF){total=pos;break;}++pos;
            if(c=='\n')break;if(c=='\r')continue;
            buf[n]=(c>=32&&c<127)?c:'?';if(buf[n]==' ')space=n;++n;
        }
        if(n==unsigned(cols)&&pos<total&&space>0){pos=start+space+1;n=space;if(f)fseek(f,pos,SEEK_SET);}
        buf[n]=0;snprintf(lines[row],sizeof lines[row],"%s",buf);
    }
    if(f)fclose(f);nextOffset=pos;more=pos<total&&index+1<128;
}
void open(Page p,bool sd,int w,int h){
    ready=sd;width=w;height=h;cols=(w-22)/12;if(cols>47)cols=47;rows=(h-92)/16;if(rows>14)rows=14;
    page=p;index=0;offsets[0]=0;filePath[0]=0;body=nullptr;listOffset=0;
    if(p==REPORTS){list();return;}
    if(p==GENERAL){body=general;strcpy(title,"GENERAL HELP");}
    if(p==POWER){body=power;strcpy(title,"POWER HELP");}
    if(p==DATA){backup=CrashReports::backupPresent(ready);body=backup?dataYes:dataNo;strcpy(title,"DATA HELP");}
    if(body)load();
}
void draw(TFT_eSPI& t){
    t.fillScreen(Theme::BG);t.setTextWrap(false);t.setTextFont(1);t.setTextSize(1);t.setTextColor(Theme::CYAN,Theme::BG);t.setCursor(8,8);
    t.print(page==HELP?"SYSTEM TROUBLESHOOTING":page==REPORTS?"CRASH REPORTS":title);
    if(page==HELP){Lang::button(t,8,35,width-16,34,"GENERAL GUIDANCE");Lang::button(t,8,75,width-16,34,"POWER HELP");Lang::button(t,8,115,width-16,34,"DATA HELP");}
    else if(page==REPORTS){
        if(!count){t.setCursor(8,40);t.print(ready?"No saved reports in /crash-reports/.":"microSD unavailable.");t.setCursor(8,58);t.print("Export occurs after a recorded reset.");}
        for(int i=0;i<count;++i){t.drawRect(8,32+i*29,width-16,26,Theme::CYAN);t.setCursor(12,40+i*29);t.print(names[i]);}
        Lang::button(t,8,height-76,(width-24)/2,30,"PREV");Lang::button(t,16+(width-24)/2,height-76,(width-24)/2,30,"NEXT");
    }else{
        t.setTextSize(2);t.setTextColor(Theme::WHITE,Theme::BG);
        for(int i=0;i<rows;++i){t.setCursor(8,30+i*16);t.print(lines[i]);}
        t.setTextSize(1);char b[48];snprintf(b,sizeof b,"Page %u  %lu%%",index+1,(unsigned long)(total?nextOffset*100/total:100));t.setCursor(8,height-58);t.print(b);
        t.fillRect(width-7,30,3,rows*16,Theme::VAPOR_PURPLE);
        int track=rows*16,thumb=track/4;int y=total?int(offsets[index]*(track-thumb)/total):0;t.fillRect(width-7,30+y,3,thumb,Theme::CYAN);
        Lang::button(t,8,height-42,(width-32)/3,32,"PREV");Lang::button(t,16+(width-32)/3,height-42,(width-32)/3,32,"NEXT");
    }
    if(page==HELP||page==REPORTS)Lang::button(t,8,height-38,width-16,30,"BACK");
    else Lang::button(t,24+2*((width-32)/3),height-42,(width-32)/3,32,"BACK");
}
bool touch(int x,int y,int w,int h){
    if(page==HELP){if(y>=h-38)return true;
        if(y>=35&&y<69)open(GENERAL,ready,w,h);else if(y>=75&&y<109)open(POWER,ready,w,h);else if(y>=115&&y<149)open(DATA,ready,w,h);return false;}
    if(page==REPORTS){if(y>=h-38)return true;
        if(y>=h-76&&y<h-46){if(x<w/2&&listOffset>0)listOffset-=4;else if(x>=w/2&&more)listOffset+=4;list();return false;}
        int i=(y-32)/29;if(y>=32&&i<count&&y<32+count*29){snprintf(filePath,sizeof filePath,"%s/crash-reports/%s",CrashReports::root(),names[i]);strcpy(title,"CRASH REPORT (READ ONLY)");body=nullptr;index=0;offsets[0]=0;page=READER;load();}return false;}
    if(y>=h-42){if(x>=2*w/3){if(page==READER){page=REPORTS;list();}else open(HELP,ready,w,h);}
        else if(x<w/3){if(index){--index;load();}}else if(more){offsets[++index]=nextOffset;load();}}
    return false;
}
}
