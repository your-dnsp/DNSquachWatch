#pragma once
#include "user_labels.h"
namespace ResearchSubmission {
// Offline reports describe ONE user-labeled observation, never an OUI rule.
// The default excludes the device-specific MAC bytes and advertised name.
bool savePair(const UserLabels::Target&,const UserLabels::Label&,uint32_t uptime,char* redactedPath,size_t capacity);
bool format(char* out,size_t capacity,const UserLabels::Target&,const UserLabels::Label&,uint32_t uptime,bool identifiers);
bool save(const UserLabels::Target&,const UserLabels::Label&,uint32_t uptime,bool identifiers,char* path,size_t capacity);
}
