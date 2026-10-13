#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <filesystem>
#include <stdexcept>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.h>
namespace fhTextureStorage {
inline winrt::hstring native(const std::filesystem::path& path) {
  auto p = path; p.make_preferred(); return winrt::hstring(p.wstring());
}
inline winrt::Windows::Storage::StorageFolder folder(const std::filesystem::path& path) {
  return winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(native(path)).get();
}
inline winrt::Windows::Storage::StorageFolder create(const std::filesystem::path& path) {
  return folder(path.parent_path()).CreateFolderAsync(native(path.filename()),
      winrt::Windows::Storage::CreationCollisionOption::OpenIfExists).get();
}
inline bool empty(const std::filesystem::path& path) {
  return folder(path).GetItemsAsync().get().Size() == 0;
}
inline void remove(const std::filesystem::path& path) {
  auto item = folder(path.parent_path()).TryGetItemAsync(native(path.filename())).get();
  if (item) item.DeleteAsync(winrt::Windows::Storage::StorageDeleteOption::PermanentDelete).get();
}
inline void rename(const std::filesystem::path& from, const std::filesystem::path& to) {
  if (from.parent_path().lexically_normal() != to.parent_path().lexically_normal())
    throw std::runtime_error("Destino de texturas no valido.");
  folder(from).RenameAsync(native(to.filename()),
      winrt::Windows::Storage::NameCollisionOption::FailIfExists).get();
}
inline bool isFolder(const std::filesystem::path& path) {
  try { return bool(folder(path)); } catch (const winrt::hresult_error&) { return false; }
}
}
