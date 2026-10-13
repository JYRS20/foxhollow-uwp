#include "settings.h"
#include <SDL3/SDL.h>
#include <stdio.h>

FhUwpSettings fhUwpDefaultSettings(void) {
  const FhUwpSettings settings = {0, 720, -1, 0, 0};
  return settings;
}

int fhUwpSettingsValid(const FhUwpSettings* settings) {
  return settings != NULL && (settings->widescreen == 0 || settings->widescreen == 1) &&
         (settings->renderHeight == 720 || settings->renderHeight == 1080) &&
         (settings->language == -1 || settings->language == 0 || settings->language == 1 ||
          settings->language == 2 || settings->language == 3 || settings->language == 5) &&
         (settings->showFps == 0 || settings->showFps == 1) &&
         settings->uiLanguage >= 0 && settings->uiLanguage < 5;
}

int fhUwpSettingsRead(const char* path, FhUwpSettings* settings) {
  *settings = fhUwpDefaultSettings();
  size_t size = 0;
  char* data = SDL_LoadFile(path, &size);
  if (data == NULL) {
    return 0;
  }
  FhUwpSettings loaded = fhUwpDefaultSettings();
  const int fields = size < 256 ? sscanf(data, "widescreen=%d\nrender_height=%d\nlanguage=%d\nshow_fps=%d\nui_language=%d",
      &loaded.widescreen, &loaded.renderHeight, &loaded.language, &loaded.showFps, &loaded.uiLanguage) : 0;
  const int ok = (fields >= 3 && fields <= 5) && fhUwpSettingsValid(&loaded);
  SDL_free(data);
  if (ok) {
    *settings = loaded;
  }
  return ok;
}

int fhUwpSettingsWrite(const char* path, const FhUwpSettings* settings) {
  if (!fhUwpSettingsValid(settings)) {
    return 0;
  }
  char* temporary = NULL;
  if (SDL_asprintf(&temporary, "%s.tmp", path) < 0) {
    return 0;
  }
  char data[128];
  const int length = snprintf(data, sizeof(data), "widescreen=%d\nrender_height=%d\nlanguage=%d\nshow_fps=%d\nui_language=%d\n",
      settings->widescreen, settings->renderHeight, settings->language, settings->showFps, settings->uiLanguage);
  int ok = length > 0 && length < (int)sizeof(data) && SDL_SaveFile(temporary, data, (size_t)length) &&
           SDL_RenamePath(temporary, path);
  if (!ok) {
    SDL_RemovePath(temporary);
  }
  SDL_free(temporary);
  return ok;
}
