#pragma once
#include "settings.h"
#include <imgui.h>
#include <string>
#include <cstdint>
#include <vector>

enum class FhMenuAction { None, Play, Resume, Restart, Browse, Textures, ToggleTextures, Settings, ExitMenu, Quit, Home, Open, Back, Refresh, Cancel, Save };
struct FhMenuFile { std::string name; bool folder; };
struct FhMenuModel {
  int page = 0;
  bool focus = true;
  bool ready = false;
  bool gameVisible = false;
  bool inGame = false;
  bool restart = false;
  bool pal = false;
  bool busy = false;
  bool importing = false;
  bool verifying = false;
  uint64_t copiedBytes = 0;
  uint64_t totalBytes = 0;
  std::string importName;
  std::string importPhase;
  bool textureMode = false;
  bool texturesEnabled = false;
  unsigned textureCount = 0;
  unsigned textureSkipped = 0;
  std::string textureName;
  bool canGoBack = false;
  FhUwpSettings settings{};
  std::string disc;
  std::string region;
  std::string location;
  std::string error;
  std::vector<FhMenuFile> files;
};
struct FhMenuCommand { FhMenuAction action = FhMenuAction::None; int index = -1; };
FhMenuCommand fhMenuDraw(FhMenuModel& model, ImTextureID logo);
void fhMenuDrawPerformance(double fps, double averageMs, double peakMs, bool toolbarOpen = false);
