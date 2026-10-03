#pragma once
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>
#include <string>
#define FILE_READ 0
#define FILE_WRITE 1
#define FILE_APPEND 2
namespace TestSD { inline std::string root;inline int writesLeft=-1;inline unsigned largestRead=0,openHandles=0,peakHandles=0,maxHandles=10000; }
class File {
 struct Data {Data(){++TestSD::openHandles;TestSD::peakHandles=std::max(TestSD::peakHandles,TestSD::openHandles);} ~Data(){--TestSD::openHandles;} std::fstream io;std::string path;bool dir=false;std::vector<std::string> children;size_t next=0;int mode=0;};
 std::shared_ptr<Data> d;
public:
 File()=default;
 File(const std::string& path,int mode){if(TestSD::openHandles>=TestSD::maxHandles)return;if(mode==FILE_READ&&!std::filesystem::exists(path))return;d=std::make_shared<Data>();d->path=path;d->mode=mode;d->dir=std::filesystem::is_directory(path);if(d->dir){for(auto& e:std::filesystem::directory_iterator(path))d->children.push_back(e.path().string());return;}auto flags=std::ios::binary|(mode==FILE_READ?std::ios::in:std::ios::out|(mode==FILE_APPEND?std::ios::app:std::ios::trunc));d->io.open(path,flags);if(!d->io.is_open())d.reset();}
 explicit operator bool()const{return bool(d);}
 bool isDirectory()const{return d&&d->dir;}
 const char* name()const{return d?d->path.c_str():"";}
 File openNextFile(){return d&&d->next<d->children.size()?File(d->children[d->next++],FILE_READ):File();}
 size_t size()const{return d&&!d->dir?std::filesystem::file_size(d->path):0;}
 size_t position(){return d?size_t(d->mode==FILE_READ?d->io.tellg():d->io.tellp()):0;}
 bool available(){return d&&!d->dir&&position()<size();}
 int read(uint8_t* out,size_t n){if(!d)return -1;TestSD::largestRead=std::max(TestSD::largestRead,unsigned(n));d->io.read((char*)out,n);return int(d->io.gcount());}
 size_t write(const uint8_t* p,size_t n){if(!d||TestSD::writesLeft==0)return 0;if(TestSD::writesLeft>0)--TestSD::writesLeft;d->io.write((const char*)p,n);return d->io?n:0;}
 bool seek(uint32_t n){if(!d)return false;d->io.clear();d->io.seekg(n);return bool(d->io);}
 void flush(){if(d&&!d->dir)d->io.flush();}
 void close(){if(d&&!d->dir)d->io.close();d.reset();}
};
struct SDClass {
 bool exists(const char* p){return std::filesystem::exists(TestSD::root+p);}
 bool mkdir(const char* p){return std::filesystem::create_directory(TestSD::root+p);}
 bool remove(const char* p){return std::filesystem::remove(TestSD::root+p);}
 bool rmdir(const char* p){return std::filesystem::remove(TestSD::root+p);}
 bool rename(const char* a,const char* b){std::error_code e;std::filesystem::rename(TestSD::root+a,TestSD::root+b,e);return !e;}
 uint64_t cardSize(){return 1024ull*1024*1024;}
 File open(const char* p,int m=FILE_READ){return File(TestSD::root+p,m);}
};
inline SDClass SD;
