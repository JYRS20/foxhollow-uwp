#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>
struct FhTexturePackResult {
  uint32_t installed = 0;
  uint32_t skipped = 0;
  uint64_t bytes = 0;
  std::vector<std::string> warnings;
};
using FhTextureProgress = std::function<void(uint64_t, uint64_t)>;
using FhTextureCancelled = std::function<bool()>;
FhTexturePackResult fhExtractTextureZip(const std::filesystem::path& zip,
    const std::filesystem::path& output, FhTextureProgress progress = {}, FhTextureCancelled cancelled = {});
