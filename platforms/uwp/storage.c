#include "storage.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <windows.h>

static char* sUserPath;
static char* sCachePath;
static char* sDiscPath;
static FILE* sLog;
static char* sSettingsPath;
static FhUwpSettings sSettings;

static char* child_path(const char* parent, const char* child) {
  char* result = NULL;
  if (SDL_asprintf(&result, "%s%s", parent, child) < 0) {
    return NULL;
  }
  return result;
}

int fhUwpStorageInit(void) {
  if (sUserPath != NULL) {
    return sLog != NULL;
  }
  sUserPath = SDL_GetPrefPath(NULL, "Foxhollow");
  if (sUserPath == NULL) {
    return 0;
  }
  sCachePath = child_path(sUserPath, "cache/");
  sDiscPath = child_path(sUserPath, "disc.iso");
  sSettingsPath = child_path(sUserPath, "settings.ini");
  char* logPath = child_path(sUserPath, "foxhollow.log");
  if (sCachePath == NULL || sDiscPath == NULL || sSettingsPath == NULL || logPath == NULL || !SDL_CreateDirectory(sCachePath)) {
    SDL_free(logPath);
    return 0;
  }
  sLog = fopen(logPath, "a");
  SDL_free(logPath);
  if (sLog == NULL) {
    return 0;
  }
  fhUwpLog("UWP", "Application starting");
  fhUwpLog("CARD", sUserPath);
  fhUwpLog("Disc", sDiscPath);
  fhUwpSettingsRead(sSettingsPath, &sSettings);
  return 1;
}

const char* fhUwpDiscPath(void) { return sDiscPath; }
int fhUwpSetDiscPath(const char* path) {
  char* selected = SDL_strdup(path);
  if (!selected) return 0;
  SDL_free(sDiscPath);
  sDiscPath = selected;
  fhUwpLog("Disc", sDiscPath);
  return 1;
}
const char* fhUwpUserPath(void) { return sUserPath; }
const char* fhUwpCachePath(void) { return sCachePath; }
const FhUwpSettings* fhUwpSettings(void) { return &sSettings; }

int fhUwpSaveSettings(const FhUwpSettings* settings) {
  if (!fhUwpSettingsWrite(sSettingsPath, settings)) {
    fhUwpLog("Settings", "Failed to save settings");
    return 0;
  }
  sSettings = *settings;
  fhUwpLog("Settings", "Settings saved");
  return 1;
}

void fhUwpLog(const char* module, const char* message) {
  char* line = NULL;
  if (SDL_asprintf(&line, "[%s] %s\n", module, message) < 0) {
    return;
  }
  OutputDebugStringA(line);
  if (sLog != NULL) {
    fputs(line, sLog);
    fflush(sLog);
  }
  SDL_free(line);
}
