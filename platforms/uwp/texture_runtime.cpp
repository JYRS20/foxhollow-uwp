#include "texture_runtime.hpp"
#include "foxhollow_mods.h"
#include "storage.h"
#include <dolphin/gx/GXStruct.h>
#include <aurora/texture.hpp>
#include <atomic>
#include "texture_storage.hpp"
#include <SDL3/SDL.h>
#include <memory>
#include <cstdio>
namespace {
std::string root;
std::atomic<bool> reload{false};
aurora::texture::ReplacementGroup group;
std::string activeFolder;
bool readTexture(void*, const char* name, std::vector<uint8_t>& output) {
  if (std::string_view(name).find_first_of("/" + std::string(1, static_cast<char>(92))) != std::string_view::npos)
    return false;
  size_t size = 0;
  const auto path = std::filesystem::path(activeFolder) / name;
  const auto utf8 = path.u8string();
  std::unique_ptr<void, decltype(&SDL_free)> data(
      SDL_LoadFile(reinterpret_cast<const char*>(utf8.c_str()), &size), SDL_free);
  if (!data) return false;
  output.assign(static_cast<const uint8_t*>(data.get()), static_cast<const uint8_t*>(data.get()) + size);
  return true;
}
void apply() {
  try {
    aurora::texture::unregister_replacements(group);
    group = {};
    const auto pack = fhReadInstalledTextures(root);
    activeFolder = pack.enabled ? pack.folder : "";
    if (pack.enabled) {
      const auto directory = fhTextureStorage::folder(pack.folder);
      for (const auto& file : directory.GetFilesAsync().get()) {
        const auto name = winrt::to_string(file.Name());
        const auto registration = aurora::texture::register_virtual_replacement(name, {readTexture, nullptr});
        if (registration.id) group.registrations.push_back(registration);
      }
    }
    const auto text = "Registered textures: " + std::to_string(group.registrations.size());
    fhUwpLog("Textures", text.c_str());
  } catch (const winrt::hresult_error& error) {
    const auto detail = winrt::to_string(error.message());
    fhUwpLog("Textures", detail.c_str());
  } catch (const std::exception& error) { fhUwpLog("Textures", error.what()); }
}
}
extern "C" void fhUwpRequestTexturesReload(void) { reload.store(true); }
extern "C" void fhModsInit(int, char**, const char* userPath) { root = userPath; reload.store(false); apply(); }
extern "C" void fhModsUpdate(void) { if (reload.exchange(false) && !root.empty()) apply(); }
extern "C" void fhModsShutdown(void) {
  aurora::texture::unregister_replacements(group); group = {}; root.clear();
}
