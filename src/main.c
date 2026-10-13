#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <aurora/event.h>
#include <aurora/main.h>
#include <dolphin/dvd.h>
#include <dolphin/gx/GXAurora.h>
#include <dolphin/vi.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_video.h>

#include "foxhollow_config.h"
#include "foxhollow_crash.h"
#include "foxhollow_mods.h"
#ifdef FOXHOLLOW_UWP
#include "storage.h"
#include "prelaunch.h"
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_timer.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int gameMain(int argc, char** argv);
void foxhollowFramePumpInit(void);

typedef struct FoxhollowDiscMapping {
  char gameName[4];
  unsigned char revision;
  const char* label;
} FoxhollowDiscMapping;

static const FoxhollowDiscMapping sDiscMappings[] = {
    {{'G', 'S', 'A', 'E'}, 0, "USA 1.0"},
    {{'G', 'S', 'A', 'E'}, 1, "USA 1.1"},
    {{'G', 'S', 'A', 'J'}, 0, "Japan 1.0"},
    {{'G', 'S', 'A', 'P'}, 0, "Europe 1.0"},
    {{'G', 'S', 'A', 'P'}, 1, "Europe 1.1"},
};

static const FoxhollowDiscMapping* disc_mapping(const DVDDiskID* id) {
  if (id == NULL || memcmp(id->company, "01", sizeof(id->company)) != 0 || id->diskNumber != 0) {
    return NULL;
  }

  for (size_t i = 0; i < sizeof(sDiscMappings) / sizeof(sDiscMappings[0]); i++) {
    const FoxhollowDiscMapping* mapping = &sDiscMappings[i];
    if (memcmp(id->gameName, mapping->gameName, sizeof(id->gameName)) == 0 && id->gameVersion == mapping->revision) {
      return mapping;
    }
  }

  return NULL;
}

static void log_callback(AuroraLogLevel level, const char* module, const char* message, unsigned int len) {
  const char* levelStr;
  FILE* out = stdout;
  switch (level) {
  case LOG_DEBUG:
    levelStr = "DEBUG";
    break;
  case LOG_INFO:
    levelStr = "INFO";
    break;
  case LOG_WARNING:
    levelStr = "WARNING";
    break;
  case LOG_ERROR:
    levelStr = "ERROR";
    out = stderr;
    break;
  case LOG_FATAL:
    levelStr = "FATAL";
    out = stderr;
    break;
  }
  fprintf(out, "[%s] %s: %s\n", levelStr, module, message);
#ifdef FOXHOLLOW_UWP
  fhUwpLog(module, message);
#endif
  if (level == LOG_FATAL) {
    fflush(out);
    abort();
  }
}

static const char* disc_path(int argc, char* argv[]) {
#ifdef FOXHOLLOW_UWP
  (void)argc;
  (void)argv;
  return fhUwpDiscPath();
#else
  if (argc > 1) {
    return argv[1];
  }
  return getenv("FOXHOLLOW_DISC");
#endif
}

static int texture_dumps_enabled(void) {
#ifdef FOXHOLLOW_UWP
  return 0;
#else
  const char* value = getenv("FOXHOLLOW_TEXTURE_DUMPS");
  if (value == NULL || value[0] == '\0') {
    return 0;
  }
  return strcmp(value, "0") != 0 && strcmp(value, "false") != 0 && strcmp(value, "off") != 0;
#endif
}

static const char* shader_cache_path(void) {
#ifdef FOXHOLLOW_UWP
  return SDL_GetBasePath();
#else
  const char* path = getenv("FOXHOLLOW_SHADER_CACHE");
  if (path == NULL || path[0] == '\0') {
    return NULL;
  }
  return path;
#endif
}

static const char* prepare_directory(const char* path) {
  if (path == NULL) {
    return NULL;
  }
  if (!SDL_CreateDirectory(path)) {
    fprintf(stderr, "foxhollow: failed to create directory %s: %s\n", path, SDL_GetError());
    return NULL;
  }
  return path;
}

int main(int argc, char* argv[]) {
  const char* disc;
  fhInstallCrashHandler();
  disc = disc_path(argc, argv);
  const FoxhollowDiscMapping* mapping;
  const int revision = fhConfigRevision();
  if (revision < 0) {
    fprintf(stderr, "foxhollow: FOXHOLLOW_REV must be 0 or 1\n");
    return 1;
  }
  if (!disc) {
    fprintf(stderr, "usage: foxhollow <path-to-disc.iso|rvz>\n(or set FOXHOLLOW_DISC)\n");
    return 1;
  }

  const AuroraConfig config = {
      .appName = "Foxhollow",
      .userPath = prepare_directory(fhConfigUserPath()),
      .cachePath = prepare_directory(fhConfigCachePath()),
      .resourcesPath = shader_cache_path(),
      .desiredBackend = fhConfigBackend(),
      .logCallback = &log_callback,
      .vsync = fhConfigVsync() != 0,
      .startFullscreen = fhConfigFullscreen() != 0,
      .allowTextureDumps = texture_dumps_enabled() != 0,
      .windowPosX = SDL_WINDOWPOS_CENTERED,
      .windowPosY = SDL_WINDOWPOS_CENTERED,
      .mem1Size = 128 * 1024 * 1024,
      .mem2Size = ARAM_DEFAULT_SIZE,
  };
  const AuroraInfo info = aurora_initialize(argc, argv, &config);
#ifdef FOXHOLLOW_UWP
  fhUwpLog("Aurora", "initialized");
  if (info.backend != BACKEND_D3D12) {
    fhUwpLog("D3D12", "Required backend unavailable");
    aurora_shutdown();
    return 1;
  }
  if (!fhUwpPrelaunch()) {
    aurora_shutdown();
    return 0;
  }
  disc = disc_path(argc, argv);
  SDL_Log("Opening selected disc: %s", disc);
  SDL_Log("Settings: aspect=%s, render_height=%d, language=%d",
          fhUwpSettings()->widescreen ? "16:9" : "4:3", fhUwpSettings()->renderHeight,
          fhUwpSettings()->language);
#endif

  AuroraSetViewportPolicy(AURORA_VIEWPORT_FIT);
  AuroraSetDisplayAspect(fhConfigDisplayAspect());
  VISetFrameBufferScale(fhConfigRenderScale());

  if (!aurora_dvd_open(disc)) {
#ifdef FOXHOLLOW_UWP
    SDL_Log("Failed to open disc image: %s", disc);
#endif
    fprintf(stderr, "foxhollow: failed to open disc image: %s\n", disc);
    aurora_shutdown();
    return 1;
  }

  mapping = disc_mapping(DVDGetCurrentDiskID());
#ifdef FOXHOLLOW_UWP
  fhUwpLog("Disc", "opened");
#endif
  if (mapping == NULL) {
    const DVDDiskID* id = DVDGetCurrentDiskID();
#ifdef FOXHOLLOW_UWP
    SDL_Log("Unsupported disc %.4s%.2s revision %u, disc %u", id->gameName, id->company,
            id->gameVersion, id->diskNumber);
#endif
    fprintf(stderr, "foxhollow: unsupported disc %.4s%.2s revision %u, disc %u\n", id->gameName, id->company,
            id->gameVersion, id->diskNumber);
    aurora_dvd_close();
    aurora_shutdown();
    return 1;
  }
  fprintf(stdout, "foxhollow: using %s disc with the GSAE01 asset mapping and USA 1.%d code\n", mapping->label,
          revision);

  fhModsInit(argc, argv, info.userPath);

  foxhollowFramePumpInit();
#ifdef FOXHOLLOW_UWP
  fhUwpLog("Foxhollow", "game initialization");
#endif
  return gameMain(argc, argv);
}
