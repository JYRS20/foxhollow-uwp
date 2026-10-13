#ifndef FOXHOLLOW_UWP_STORAGE_H
#define FOXHOLLOW_UWP_STORAGE_H
#include "settings.h"

#ifdef __cplusplus
extern "C" {
#endif

int fhUwpStorageInit(void);
const char* fhUwpDiscPath(void);
int fhUwpSetDiscPath(const char* path);
const char* fhUwpUserPath(void);
const char* fhUwpCachePath(void);
void fhUwpLog(const char* module, const char* message);
const FhUwpSettings* fhUwpSettings(void);
int fhUwpSaveSettings(const FhUwpSettings* settings);

#ifdef __cplusplus
}
#endif
#endif
