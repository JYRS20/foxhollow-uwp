#pragma once
#include <string>
struct FhInstalledTextures {
  std::string folder;
  std::string name;
  unsigned count = 0;
  unsigned skipped = 0;
  bool enabled = false;
};
FhInstalledTextures fhReadInstalledTextures(const std::string& userPath);
extern "C" void fhUwpRequestTexturesReload(void);
