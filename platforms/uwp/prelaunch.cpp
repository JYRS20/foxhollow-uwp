#include "prelaunch.h"
#include "disc_browser.hpp"
#include "menu_view.hpp"
#include "frame_stats.hpp"
#include "texture_runtime.hpp"
#include "storage.h"
#include <aurora/aurora.h>
#include <aurora/event.h>
#include <aurora/imgui.h>
#include <imgui.h>
#include <SDL3/SDL.h>
#include <png.h>
#include <vector>
#include <memory>
#include <atomic>
#include <dolphin/pad.h>
#include <dolphin/vi.h>
#include <dolphin/gx.h>
#include "foxhollow_config.h"
#include "foxhollow_quit.h"
#include <winrt/Windows.ApplicationModel.Core.h>

static ImTextureID menu_logo() {
  const char* base = SDL_GetBasePath();
  if (!base) return 0;
  const auto path = std::string(base) + "Assets/FoxhollowLogo.png";
  png_image image{};
  image.version = PNG_IMAGE_VERSION;
  if (!png_image_begin_read_from_file(&image, path.c_str())) return 0;
  image.format = PNG_FORMAT_RGBA;
  std::vector<unsigned char> pixels(PNG_IMAGE_SIZE(image));
  ImTextureID texture = 0;
  if (png_image_finish_read(&image, nullptr, pixels.data(), 0, nullptr)) {
    for (size_t i = 0; i + 3 < pixels.size(); i += 4)
      if (pixels[i] <= 12 && pixels[i + 1] <= 12 && pixels[i + 2] <= 12) pixels[i + 3] = 0;
    texture = aurora_imgui_add_texture(image.width, image.height, pixels.data());
  }
  png_image_free(&image);
  return texture;
}


namespace {
std::unique_ptr<FhDiscBrowser> browser;
FhMenuModel model;
ImTextureID logo = 0;
ImFont* previousFont = nullptr;
ImFont* menuFont = nullptr;
ImGuiConfigFlags previousFlags = 0;
bool visible = true;
bool gameStarted = false;
unsigned importGeneration = 0;
unsigned textureGeneration = 0;
bool pendingDisc = false;
int bootLanguage = -1;
std::string settingsError;
std::atomic<int> restartResult{-1};

FhFrameStats frameStats;
void drawPerformance() {
  if (!fhUwpSettings()->showFps) { frameStats = {}; return; }
  frameStats.sample(SDL_GetTicksNS());
  fhMenuDrawPerformance(frameStats.fps, frameStats.averageMs, frameStats.peakMs, visible && model.inGame);
}
void home() { model.page = 0; model.focus = true; }
void set_visible(bool value) {
  visible = value;
  auto& io = ImGui::GetIO();
  io.ConfigFlags = previousFlags | ImGuiConfigFlags_NavEnableGamepad | ImGuiConfigFlags_NavEnableKeyboard;
  io.FontDefault = value ? menuFont : previousFont;
  io.ConfigNavCursorVisibleAlways = value;
  PADBlockInput(value);
  if (value) home();
  else ImGui::SetWindowFocus(nullptr);
}
bool save() {
  const FhUwpSettings previous = *fhUwpSettings();
  if (!model.pal) model.settings.language = -1;
  if (!fhUwpSaveSettings(&model.settings)) {
    settingsError = "No se pudieron guardar las opciones.";
    return false;
  }
  settingsError.clear();
  model.restart = gameStarted && (pendingDisc || model.settings.language != bootLanguage);
  if (gameStarted) {
    if (previous.widescreen != model.settings.widescreen)
      AuroraSetDisplayAspect(fhConfigDisplayAspect());
    if (previous.renderHeight != model.settings.renderHeight)
      VISetFrameBufferScale(fhConfigRenderScale());
  }
  return true;
}
void restart() {
  try {
    winrt::Windows::ApplicationModel::Core::CoreApplication::RequestRestartAsync(L"").Completed(
        [](const auto& operation, auto) {
          try { restartResult.store(static_cast<int>(operation.GetResults())); }
          catch (...) { restartResult.store(3); }
        });
  } catch (...) { restartResult.store(3); }
}
void draw(bool gameVisible) {
  const int result = restartResult.exchange(-1);
  if (result >= 0) settingsError = "Reinicio: " + std::to_string(result);
  const auto state = browser->snapshot();
  model.gameVisible = gameVisible;
  model.ready = state.disc.valid;
  model.pal = state.disc.pal();
  model.busy = state.busy;
  model.importing = state.importing;
  model.verifying = state.verifying;
  model.copiedBytes = state.copiedBytes;
  model.totalBytes = state.totalBytes;
  model.importName = state.importName;
  model.importPhase = state.importPhase;
  model.textureMode = state.textureMode;
  model.texturesEnabled = state.texturesEnabled;
  model.textureCount = state.textureCount;
  model.textureSkipped = state.textureSkipped;
  model.textureName = state.textureName;
  if (state.textureGeneration != textureGeneration) {
    textureGeneration = state.textureGeneration;
    fhUwpRequestTexturesReload();
  }
  model.canGoBack = state.canGoBack;
  model.disc = state.disc.label;
  model.location = state.location;
  model.error = state.error.empty() ? settingsError : state.error;
  model.region = state.disc.gameId.size() > 3 && state.disc.gameId[3] == 'P' ? "EUR" :
      state.disc.gameId.size() > 3 && state.disc.gameId[3] == 'J' ? "JPN" : "USA";
  model.files.clear();
  for (const auto& entry : state.entries) model.files.push_back({entry.name, static_cast<bool>(entry.folder)});
  if (state.importGeneration != importGeneration) {
    importGeneration = state.importGeneration;
    if (gameStarted) { pendingDisc = true; model.restart = true; }
    home();
  }
  const bool popup = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
  if (!popup && !state.busy && (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight, false) ||
      ImGui::IsKeyPressed(ImGuiKey_Escape, false))) {
    if ((model.page == 2 || model.page == 4) && state.canGoBack) browser->back();
    else if (model.page != 0) home();
    else if (model.inGame && save()) set_visible(false);
  }
  const auto command = fhMenuDraw(model, logo);
  switch (command.action) {
  case FhMenuAction::Play:
    if (state.disc.valid && !state.busy && save()) {
      if (model.restart) restart();
      else if (gameStarted) { model.inGame = true; set_visible(false); }
    }
    break;
  case FhMenuAction::Resume:
    if (!state.busy && save()) set_visible(false);
    break;
  case FhMenuAction::Restart:
    if (!state.busy && save()) restart();
    break;
  case FhMenuAction::Browse:
    if (!model.inGame) { browser->setTextureMode(false); model.page = 2; model.focus = true; browser->refresh(); }
    break;
  case FhMenuAction::Textures:
    if (model.inGame) { model.page = 5; model.focus = true; }
    else { browser->setTextureMode(true); model.page = 4; model.focus = true; browser->refresh(); }
    break;
  case FhMenuAction::ToggleTextures:
    if (!state.busy) browser->toggleTextures();
    break;
  case FhMenuAction::Settings:
    model.page = 1; model.focus = true; break;
  case FhMenuAction::ExitMenu:
    model.page = 3; model.focus = true; break;
  case FhMenuAction::Quit:
    browser->cancel(); fhRequestQuit(); break;
  case FhMenuAction::Home:
    home(); break;
  case FhMenuAction::Save:
    if (save()) home(); break;
  case FhMenuAction::Open:
    if (!state.busy && command.index >= 0 && command.index < static_cast<int>(state.entries.size()))
      browser->open(state.entries[command.index]);
    break;
  case FhMenuAction::Back:
    browser->back(); break;
  case FhMenuAction::Refresh:
    browser->refresh(); break;
  case FhMenuAction::Cancel:
    browser->cancel(); break;
  case FhMenuAction::None:
    break;
  }
}
}

extern "C" int fhUwpPrelaunch(void) {
  browser = std::make_unique<FhDiscBrowser>(fhUwpUserPath());
  browser->refresh();
  model.settings = *fhUwpSettings();
  bootLanguage = model.settings.language;
  auto& io = ImGui::GetIO();
  previousFlags = io.ConfigFlags;
  previousFont = io.FontDefault ? io.FontDefault :
      io.Fonts->Fonts.Size ? io.Fonts->Fonts[0] : io.Fonts->AddFontDefault();
  menuFont = previousFont;
  if (const char* base = SDL_GetBasePath()) {
    const auto path = std::string(base) + "Assets/MenuFont.ttf";
    if (auto* font = io.Fonts->AddFontFromFileTTF(path.c_str(), 32.f)) menuFont = font;
  }
  logo = menu_logo();
  set_visible(true);
  fhUwpLog("Menu", "Overlay initialized");
  while (!fhQuitRequested()) {
    const auto state = browser->snapshot();
    if (state.disc.valid && !state.busy && !state.importing) {
      if (!fhUwpSetDiscPath(state.discPath.c_str())) {
        settingsError = "No se pudo establecer la ruta del disco.";
        break;
      }
      gameStarted = true;
      importGeneration = state.importGeneration;
      bootLanguage = fhUwpSettings()->language;
      model.focus = true;
      model.page = 0;
      fhUwpLog("Menu", "Starting background game");
      return 1;
    }
    for (const auto* event = aurora_update(); event && event->type != AURORA_NONE; ++event)
      if (event->type == AURORA_EXIT) fhRequestQuit();
    if (fhQuitRequested()) break;
    if (!aurora_begin_frame()) { SDL_Delay(16); continue; }
    draw(false);
    drawPerformance();
    aurora_end_frame();
    SDL_Delay(16);
  }
  browser.reset();
  set_visible(false);
  return 0;
}

extern "C" void fhUwpMenuDraw(int gameVisible) {
  if (model.inGame && (ImGui::IsKeyPressed(ImGuiKey_GamepadBack, false) ||
      ImGui::IsKeyPressed(ImGuiKey_F1, false))) {
    if (visible) {
      if (!browser->snapshot().busy && save()) set_visible(false);
    } else {
      model.settings = *fhUwpSettings();
      set_visible(true);
    }
  }
  if (visible) draw(gameVisible != 0);
  drawPerformance();
}

extern "C" int fhUwpTexturesPreparing(void) {
  if (!browser) return 0;
  const auto state = browser->snapshot();
  return state.textureMode && state.busy && state.importing;
}
