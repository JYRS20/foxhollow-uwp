#pragma once
#include <cstdint>
#include <algorithm>
struct FhFrameStats {
  double fps = 0;
  double averageMs = 0;
  double peakMs = 0;
  bool started = false;
  uint64_t previous = 0;
  uint64_t elapsed = 0;
  uint64_t peak = 0;
  uint64_t frames = 0;
  void sample(uint64_t now) {
    if (!started) { started = true; previous = now; return; }
    if (now <= previous) return;
    const auto delta = now - previous;
    previous = now;
    elapsed += delta;
    peak = std::max(peak, delta);
    ++frames;
    if (elapsed >= 500000000) {
      fps = static_cast<double>(frames) * 1000000000.0 / elapsed;
      averageMs = static_cast<double>(elapsed) / frames / 1000000.0;
      peakMs = static_cast<double>(peak) / 1000000.0;
      elapsed = peak = frames = 0;
    }
  }
};
