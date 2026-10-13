#include <SDL3/SDL.h>
#include "../../extern/aurora/lib/system_info.hpp"
#include "../../extern/aurora/lib/internal.hpp"

namespace aurora {
void log_system_information() {
  Module log("aurora::system_info");
  log.info("UWP x64, SDL {}, CPUs {}, RAM {} MiB", SDL_GetVersion(), SDL_GetNumLogicalCPUCores(), SDL_GetSystemRAM());
}
}
