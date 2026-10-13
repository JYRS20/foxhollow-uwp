#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <windows.h>
#include "storage.h"

extern "C" int aurora_main(int argc, char** argv);

static void SDLCALL sdl_log(void*, int, SDL_LogPriority, const char* message) {
  fhUwpLog("SDL", message);
}

static int bootstrap(int argc, char** argv) {
  SDL_SetHint(SDL_HINT_JOYSTICK_WGI, "1");
  if (!fhUwpStorageInit()) {
    OutputDebugStringA("[UWP] Storage initialization failed\n");
    return 1;
  }
  SDL_SetLogOutputFunction(sdl_log, nullptr);
  fhUwpLog("UWP", "0.5.8.16");
  return aurora_main(argc, argv);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
  return SDL_RunApp(0, nullptr, bootstrap, nullptr);
}
