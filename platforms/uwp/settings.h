#ifndef FOXHOLLOW_UWP_SETTINGS_H
#define FOXHOLLOW_UWP_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int widescreen;
  int renderHeight;
  int language;
  int showFps;
  int uiLanguage;
} FhUwpSettings;

FhUwpSettings fhUwpDefaultSettings(void);
int fhUwpSettingsValid(const FhUwpSettings* settings);
int fhUwpSettingsRead(const char* path, FhUwpSettings* settings);
int fhUwpSettingsWrite(const char* path, const FhUwpSettings* settings);

#ifdef __cplusplus
}
#endif
#endif
