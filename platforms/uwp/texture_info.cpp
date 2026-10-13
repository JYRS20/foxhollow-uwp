#include "texture_runtime.hpp"
#include "texture_storage.hpp"
#include <SDL3/SDL.h>
#include <sstream>
#include <fstream>
FhInstalledTextures fhReadInstalledTextures(const std::string& userPath) {
  FhInstalledTextures result;
  size_t length = 0;
  void* data = SDL_LoadFile((std::filesystem::path(userPath) / "texture-pack.ini").string().c_str(), &length);
  if (!data) return result;
  std::istringstream file(std::string(static_cast<const char*>(data), length));
  SDL_free(data);
  std::string folder;
  if (!std::getline(file, folder)) return result;
  if (folder.size() >= 3 && static_cast<unsigned char>(folder[0]) == 0xef &&
      static_cast<unsigned char>(folder[1]) == 0xbb && static_cast<unsigned char>(folder[2]) == 0xbf) folder.erase(0, 3);
  if (!folder.starts_with("pack-") ||
      folder.find_first_not_of("pack-0123456789") != std::string::npos) return result;
  if (!std::getline(file, result.name) || !(file >> result.count >> result.skipped)) return {};
  const auto path = std::filesystem::path(userPath) / "texture_packs" / folder;
  if (!fhTextureStorage::isFolder(path)) return {};
  result.folder = path.string();
  SDL_PathInfo flag{};
  result.enabled = !SDL_GetPathInfo((std::filesystem::path(userPath) / "textures-disabled.flag").string().c_str(), &flag);
  return result;
}
