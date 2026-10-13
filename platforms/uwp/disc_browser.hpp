#ifndef FOXHOLLOW_UWP_DISC_BROWSER_HPP
#define FOXHOLLOW_UWP_DISC_BROWSER_HPP

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
#include <mutex>
#include <string>
#include <cstdint>
#include <thread>
#include <vector>

struct FhDiscInfo {
  bool valid = false;
  std::string gameId;
  int revision = 0;
  std::string label;
  bool pal() const { return valid && gameId.size() == 6 && gameId[3] == 'P'; }
};

FhDiscInfo fhUwpProbeDisc(const char* path);

struct FhDiscEntry {
  std::string name;
  winrt::Windows::Storage::StorageFolder folder{nullptr};
  winrt::Windows::Storage::StorageFile file{nullptr};
};

struct FhBrowserSnapshot {
  bool busy = false;
  bool importing = false;
  bool verifying = false;
  uint64_t copiedBytes = 0;
  uint64_t totalBytes = 0;
  std::string importName;
  std::string importPhase;
  bool textureMode = false;
  bool texturesEnabled = false;
  unsigned textureGeneration = 0;
  unsigned textureCount = 0;
  unsigned textureSkipped = 0;
  std::string textureName;
  bool canGoBack = false;
  unsigned importGeneration = 0;
  std::string location;
  std::string error;
  std::vector<FhDiscEntry> entries;
  FhDiscInfo disc;
  std::string discPath;
};

class FhDiscBrowser {
public:
  explicit FhDiscBrowser(std::string userPath);
  ~FhDiscBrowser();
  FhBrowserSnapshot snapshot();
  void refresh();
  void open(const FhDiscEntry& entry);
  void back();
  void cancel();
  void setTextureMode(bool value);
  void toggleTextures();

private:
  template<class Task> void start(Task task, bool importing = false);
  void list(winrt::Windows::Storage::StorageFolder folder, bool pushHistory);
  void import(winrt::Windows::Storage::StorageFile file);
  void importTextures(winrt::Windows::Storage::StorageFile file);
  std::string mUserPath;
  std::mutex mMutex;
  std::thread mWorker;
  FhBrowserSnapshot mState;
  winrt::Windows::Storage::StorageFolder mFolder{nullptr};
  std::vector<winrt::Windows::Storage::StorageFolder> mHistory;
  winrt::Windows::Foundation::IAsyncOperationWithProgress<uint64_t, uint64_t> mCopy{nullptr};
  bool mCancelled = false;
};
#endif
