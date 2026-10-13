#include "disc_browser.hpp"
#include "storage.h"
#include "texture_zip.hpp"
#include "texture_storage.hpp"
#include "texture_runtime.hpp"
#include <filesystem>
#include <chrono>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>
#include <SDL3/SDL.h>
#include <nod.h>
#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>

using namespace winrt::Windows::Storage;

namespace {
struct DiscStream {
  SDL_IOStream* io;
};

int64_t read_at(void* data, uint64_t offset, void* output, size_t size) {
  auto* stream = static_cast<DiscStream*>(data);
  if (SDL_SeekIO(stream->io, static_cast<Sint64>(offset), SDL_IO_SEEK_SET) < 0) {
    return -1;
  }
  const auto count = SDL_ReadIO(stream->io, output, size);
  return count == 0 && SDL_GetIOStatus(stream->io) == SDL_IO_STATUS_ERROR ? -1 : static_cast<int64_t>(count);
}

int64_t stream_size(void* data) {
  return SDL_GetIOSize(static_cast<DiscStream*>(data)->io);
}

void stream_close(void* data) {
  auto* stream = static_cast<DiscStream*>(data);
  SDL_CloseIO(stream->io);
  stream->io = nullptr;
}

bool disc_extension(const std::string& name) {
  const auto dot = name.find_last_of('.');
  if (dot == std::string::npos) {
    return false;
  }
  const char* extension = name.c_str() + dot;
  return SDL_strcasecmp(extension, ".iso") == 0 || SDL_strcasecmp(extension, ".rvz") == 0 ||
         SDL_strcasecmp(extension, ".gcm") == 0;
}
}

FhDiscInfo fhUwpProbeDisc(const char* path) {
  FhDiscInfo info;
  DiscStream stream{SDL_IOFromFile(path, "rb")};
  if (stream.io == nullptr) {
    return info;
  }
  NodDiscStream descriptor{&stream, read_at, stream_size, stream_close};
  NodDiscOptions options{};
  NodHandle* handle = nullptr;
  const auto result = nod_disc_open_stream(&descriptor, &options, &handle);
  if (result == NOD_RESULT_OK && handle != nullptr) {
    std::array<uint8_t, 32> header{};
    if (nod_read(handle, header.data(), header.size()) == header.size()) {
      info.gameId.assign(reinterpret_cast<const char*>(header.data()), 6);
      info.revision = header[7];
      info.valid = header[6] == 0 &&
          ((info.gameId == "GSAE01" && info.revision <= 1) ||
           (info.gameId == "GSAP01" && info.revision <= 1) ||
           (info.gameId == "GSAJ01" && info.revision == 0));
      if (info.valid) {
        const char* region = info.gameId[3] == 'P' ? "Europa" : info.gameId[3] == 'J' ? "Japon" : "USA";
        info.label = std::string(region) + " 1." + std::to_string(info.revision);
      }
    }
  }
  nod_free(handle);
  if (stream.io != nullptr) {
    stream_close(&stream);
  }
  return info;
}

FhDiscBrowser::FhDiscBrowser(std::string userPath) : mUserPath(std::move(userPath)) {
  mState.location = "Dispositivos USB";
  size_t length = 0;
  if (void* data = SDL_LoadFile((mUserPath + "disc-path.txt").c_str(), &length)) {
    mState.discPath.assign(static_cast<const char*>(data), length);
    SDL_free(data);
  } else {
    mState.discPath = mUserPath + "disc.iso";
  }
  const auto pack = fhReadInstalledTextures(mUserPath);
  mState.textureName = pack.name;
  mState.textureCount = pack.count;
  mState.textureSkipped = pack.skipped;
  mState.texturesEnabled = pack.enabled;
}

FhDiscBrowser::~FhDiscBrowser() {
  cancel();
  if (mWorker.joinable()) {
    mWorker.join();
  }
}

FhBrowserSnapshot FhDiscBrowser::snapshot() {
  std::lock_guard lock(mMutex);
  return mState;
}

template<class Task> void FhDiscBrowser::start(Task task, bool importing) {
  {
    std::lock_guard lock(mMutex);
    if (mState.busy) {
      return;
    }
    mState.busy = true;
    mState.importing = importing;
    mState.error.clear();
    mState.verifying = false;
    mState.copiedBytes = 0;
    mState.totalBytes = 0;
    mState.importName.clear();
    mState.importPhase.clear();
    mCancelled = false;
  }
  if (mWorker.joinable()) {
    mWorker.join();
  }
  mWorker = std::thread([this, task = std::move(task)] {
    bool apartment = false;
    try {
      winrt::init_apartment(winrt::apartment_type::multi_threaded);
      apartment = true;
      task();
    } catch (const winrt::hresult_error& error) {
      const auto detail = winrt::to_string(error.message()) + " (" + std::to_string(error.code().value) + ")";
      fhUwpLog("USB", detail.c_str());
      std::lock_guard lock(mMutex);
      mState.error = mCancelled ? "Importacion cancelada." : "No se pudo acceder al archivo o dispositivo USB. Revisa foxhollow.log.";
    } catch (const std::exception& error) {
      fhUwpLog("Disc", error.what());
      std::lock_guard lock(mMutex);
      mState.error = error.what();
    }
    {
      std::lock_guard lock(mMutex);
      mCopy = nullptr;
      mState.busy = false;
      mState.importing = false;
      mState.verifying = false;
    }
    if (apartment) {
      winrt::uninit_apartment();
    }
  });
}

void FhDiscBrowser::refresh() {
  StorageFolder folder{nullptr};
  {
    std::lock_guard lock(mMutex);
    folder = mFolder;
  }
  start([this, folder] {
    if (!folder && !snapshot().textureMode) {
      const auto selected = snapshot().discPath;
      auto disc = fhUwpProbeDisc(selected.c_str());
      std::lock_guard lock(mMutex);
      mState.disc = std::move(disc);
      if (!mState.disc.valid && selected != mUserPath + "disc.iso")
        mState.error = "Conecta la USB del juego o selecciona otro disco.";
    }
    list(folder, false);
  });
}

void FhDiscBrowser::list(StorageFolder folder, bool pushHistory) {
  const bool textures = snapshot().textureMode;
  std::vector<FhDiscEntry> entries;
  std::string location = "Dispositivos USB";
  if (folder) {
    location = winrt::to_string(folder.Path());
    for (const auto& child : folder.GetFoldersAsync().get()) {
      entries.push_back({winrt::to_string(child.Name()), child, nullptr});
    }
    for (const auto& file : folder.GetFilesAsync().get()) {
      auto name = winrt::to_string(file.Name());
      if (textures ? (name.size() > 4 && SDL_strcasecmp(name.c_str() + name.size() - 4, ".zip") == 0) : disc_extension(name)) {
        entries.push_back({std::move(name), nullptr, file});
      }
    }
    fhUwpLog("USB", location.c_str());
  } else {
    fhUwpLog("USB", "Scanning removable devices");
    for (const auto& device : KnownFolders::RemovableDevices().GetFoldersAsync().get()) {
      entries.push_back({winrt::to_string(device.Name()), device, nullptr});
    }
    const auto message = "Devices found: " + std::to_string(entries.size());
    fhUwpLog("USB", message.c_str());
  }
  std::sort(entries.begin(), entries.end(), [](const FhDiscEntry& a, const FhDiscEntry& b) {
    if (static_cast<bool>(a.folder) != static_cast<bool>(b.folder)) {
      return static_cast<bool>(a.folder);
    }
    return SDL_strcasecmp(a.name.c_str(), b.name.c_str()) < 0;
  });
  std::lock_guard lock(mMutex);
  if (pushHistory) {
    mHistory.push_back(mFolder);
  }
  mFolder = folder;
  mState.canGoBack = static_cast<bool>(folder);
  mState.location = std::move(location);
  mState.entries = std::move(entries);
}

void FhDiscBrowser::open(const FhDiscEntry& entry) {
  if (entry.folder) {
    start([this, folder = entry.folder] { list(folder, true); });
  } else if (entry.file) {
    const bool textures = snapshot().textureMode;
    start([this, file = entry.file, textures] { if (textures) importTextures(file); else import(file); }, textures);
  }
}

void FhDiscBrowser::back() {
  StorageFolder folder{nullptr};
  {
    std::lock_guard lock(mMutex);
    if (mState.busy || mHistory.empty()) {
      return;
    }
    folder = mHistory.back();
    mHistory.pop_back();
  }
  start([this, folder] { list(folder, false); });
}

void FhDiscBrowser::import(StorageFile file) {
  const auto name = winrt::to_string(file.Name());
  const auto path = winrt::to_string(file.Path());
  if (path.empty()) throw std::runtime_error("El disco USB no tiene una ruta accesible.");
  {
    std::lock_guard lock(mMutex);
    mState.importing = true;
    mState.verifying = true;
    mState.importName = name;
    mState.importPhase = "Comprobando disco en USB...";
  }
  auto info = fhUwpProbeDisc(path.c_str());
  if (!info.valid) throw std::runtime_error("No se pudo abrir un disco compatible desde USB. Revisa foxhollow.log.");
  std::string user = mUserPath;
  std::replace(user.begin(), user.end(), '/', static_cast<char>(92));
  while (user.size() > 3 && user.back() == static_cast<char>(92)) user.pop_back();
  const auto destination = StorageFolder::GetFolderFromPathAsync(winrt::to_hstring(user)).get();
  const auto pointer = destination.CreateFileAsync(L"disc-path.tmp", CreationCollisionOption::ReplaceExisting).get();
  FileIO::WriteBytesAsync(pointer, winrt::array_view<const uint8_t>(
      reinterpret_cast<const uint8_t*>(path.data()), reinterpret_cast<const uint8_t*>(path.data() + path.size()))).get();
  try {
    std::lock_guard lock(mMutex);
    if (mCancelled) throw std::runtime_error("Seleccion cancelada.");
    pointer.RenameAsync(L"disc-path.txt", NameCollisionOption::ReplaceExisting).get();
    mState.disc = std::move(info);
    mState.discPath = path;
    ++mState.importGeneration;
  } catch (...) {
    SDL_RemovePath((mUserPath + "disc-path.tmp").c_str());
    throw;
  }
  fhUwpLog("Disc", ("Selected USB disc (no copy): " + path).c_str());
}

void FhDiscBrowser::cancel() {
  winrt::Windows::Foundation::IAsyncOperationWithProgress<uint64_t, uint64_t> operation{nullptr};
  {
    std::lock_guard lock(mMutex);
    mCancelled = true;
    operation = mCopy;
  }
  if (operation) {
    operation.Cancel();
  }
}

void FhDiscBrowser::setTextureMode(bool value) {
  std::lock_guard lock(mMutex);
  if (mState.busy) return;
  mState.textureMode = value;
  mFolder = nullptr;
  mHistory.clear();
  mState.entries.clear();
  mState.canGoBack = false;
  mState.location = "Dispositivos USB";
}
void FhDiscBrowser::toggleTextures() {
  std::lock_guard lock(mMutex);
  if (mState.busy || !mState.textureCount) return;
  const auto flag = mUserPath + "textures-disabled.flag";
  const bool ok = mState.texturesEnabled ? SDL_SaveFile(flag.c_str(), "1", 1) : SDL_RemovePath(flag.c_str());
  if (!ok) { mState.error = "No se pudo cambiar el estado de las texturas."; return; }
  mState.texturesEnabled = !mState.texturesEnabled;
  ++mState.textureGeneration;
}
void FhDiscBrowser::importTextures(StorageFile file) {
  const auto temporary = mUserPath + "texture-import.zip";
  std::string user = mUserPath;
  std::replace(user.begin(), user.end(), '/', static_cast<char>(92));
  while (user.size() > 3 && user.back() == static_cast<char>(92)) user.pop_back();
  const auto destination = StorageFolder::GetFolderFromPathAsync(winrt::to_hstring(user)).get();
  const auto packFolder = destination.CreateFolderAsync(L"texture_packs", CreationCollisionOption::OpenIfExists).get();
  const auto packs = std::filesystem::path(packFolder.Path().c_str()).lexically_normal();
  const auto stage = packs / "importing";
  auto safeRemove = [&](const std::filesystem::path& target) {
    if (target.lexically_normal().parent_path() != packs)
      throw std::runtime_error("Ruta temporal de texturas no valida.");
    fhTextureStorage::remove(target);
  };
  std::filesystem::path final;
  bool committed = false;
  try {
    const auto name = winrt::to_string(file.Name());
    {
      std::lock_guard lock(mMutex);
      mState.importing = true;
      mState.importName = name;
      mState.importPhase = "Copiando ZIP desde USB...";
    }
    const auto copied = destination.CreateFileAsync(L"texture-import.zip", CreationCollisionOption::ReplaceExisting).get();
    {
      using namespace winrt::Windows::Storage::Streams;
      const auto input = file.OpenAsync(FileAccessMode::Read).get();
      const auto output = copied.OpenAsync(FileAccessMode::ReadWrite).get();
      const auto total = input.Size();
      { std::lock_guard lock(mMutex); mState.totalBytes = total; }
      auto operation = RandomAccessStream::CopyAsync(input, output);
      operation.Progress([this](const auto&, uint64_t bytes) {
        std::lock_guard lock(mMutex);
        if (mState.importPhase == "Copiando ZIP desde USB...") mState.copiedBytes = std::min(bytes, mState.totalBytes);
      });
      { std::lock_guard lock(mMutex); mCopy = operation; if (mCancelled) operation.Cancel(); }
      if (operation.get() != total || !output.FlushAsync().get()) throw std::runtime_error("No se pudo copiar el ZIP completo.");
      input.Close(); output.Close();
      { std::lock_guard lock(mMutex); mCopy = nullptr; mState.copiedBytes = 0; mState.totalBytes = 0;
        mState.importPhase = "Extrayendo y comprobando texturas..."; }
    }
    safeRemove(stage);
    const auto result = fhExtractTextureZip(temporary, stage,
        [this](uint64_t done, uint64_t total) { std::lock_guard lock(mMutex); mState.copiedBytes = done; mState.totalBytes = total; },
        [this] { std::lock_guard lock(mMutex); return mCancelled; });
    const auto token = std::chrono::system_clock::now().time_since_epoch().count();
    const auto folderName = "pack-" + std::to_string(token);
    final = packs / folderName;
    { std::lock_guard lock(mMutex); if (mCancelled) throw std::runtime_error("Importacion cancelada.");
      mState.verifying = true; mState.importPhase = "Activando el pack..."; }
    fhTextureStorage::rename(stage, final);
    const auto metadata = folderName + "\n" + name + "\n" + std::to_string(result.installed) + "\n" + std::to_string(result.skipped) + "\n";
    const auto pointer = destination.CreateFileAsync(L"texture-pack.ini.tmp", CreationCollisionOption::ReplaceExisting).get();
    FileIO::WriteTextAsync(pointer, winrt::to_hstring(metadata)).get();
    {
      std::lock_guard lock(mMutex);
      if (mCancelled) throw std::runtime_error("Importacion cancelada.");
      pointer.RenameAsync(L"texture-pack.ini", NameCollisionOption::ReplaceExisting).get();
      committed = true;
      const auto flag = mUserPath + "textures-disabled.flag";
      fhTextureStorage::remove(flag);
      const auto active = fhReadInstalledTextures(mUserPath);
      mState.textureName = active.name;
      mState.textureCount = active.count;
      mState.textureSkipped = active.skipped;
      mState.texturesEnabled = active.enabled;
      ++mState.textureGeneration;
    }
    const auto summary = "Imported " + std::to_string(result.installed) + " textures, skipped " + std::to_string(result.skipped);
    fhUwpLog("Textures", summary.c_str());
    for (const auto& warning : result.warnings) fhUwpLog("Textures", warning.c_str());
    SDL_RemovePath(temporary.c_str());
  } catch (...) {
    SDL_RemovePath(temporary.c_str());
    try {
      safeRemove(stage);
      if (!committed && !final.empty()) safeRemove(final);
    } catch (const winrt::hresult_error& cleanup) {
      const auto detail = winrt::to_string(cleanup.message());
      fhUwpLog("Textures cleanup", detail.c_str());
    } catch (const std::exception& cleanup) { fhUwpLog("Textures cleanup", cleanup.what()); }
    throw;
  }
}
