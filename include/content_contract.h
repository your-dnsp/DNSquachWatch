#pragma once
#include <cstring>
#include <cctype>
namespace ContentContract {
// Unknown/missing requirements fail closed. No download of card assets occurs.
inline bool supported(const char* json){
 if(!json)return false;const char* key="\"required_content\"";const char* p=strstr(json,key);if(!p||strstr(p+strlen(key),key))return false;
 p+=strlen(key);while(*p&&isspace((unsigned char)*p))++p;if(*p++!=':')return false;
 while(*p&&isspace((unsigned char)*p))++p;return !strncmp(p,"\"v1.5\"",6);
}
inline bool imageDigest(const char* json,unsigned char out[32]) {
 const char* key="\"signed_image_sha256\"";const char* p=strstr(json,key);
 if(!p||strstr(p+strlen(key),key))return false;p+=strlen(key);
 while(*p&&isspace((unsigned char)*p))++p;if(*p++!=':')return false;
 while(*p&&isspace((unsigned char)*p))++p;if(*p++!='"')return false;
 for(unsigned i=0;i<32;++i){unsigned v=0;for(unsigned j=0;j<2;++j){char c=*p++;if(c>='0'&&c<='9')v=v*16+c-'0';else if(c>='a'&&c<='f')v=v*16+c-'a'+10;else return false;}out[i]=v;}
 return *p=='"';
}
}
