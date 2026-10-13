#include <SDL3/SDL.h>
#include <webgpu/webgpu_cpp.h>
#include "storage.h"

extern "C" __declspec(dllimport) void* uwp_GetWindowReference();

extern "C" int aurora_main(int, char**) {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    fhUwpLog("SDL", SDL_GetError());
    return 1;
  }
  fhUwpLog("SDL", "initialized");
  SDL_Window* window = SDL_CreateWindow("Foxhollow UWP", 1920, 1080, SDL_WINDOW_FULLSCREEN);
  if (window == nullptr) {
    fhUwpLog("SDL", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  int result = 1;
  {
    const wgpu::InstanceFeatureName feature = wgpu::InstanceFeatureName::TimedWaitAny;
    wgpu::InstanceDescriptor descriptor{};
    descriptor.requiredFeatureCount = 1;
    descriptor.requiredFeatures = &feature;
    auto instance = wgpu::CreateInstance(&descriptor);
    if (!instance) {
      fhUwpLog("Dawn", "Instance creation failed");
      SDL_DestroyWindow(window);
      SDL_Quit();
      return 1;
    }
    wgpu::SurfaceDescriptorFromWindowsCoreWindow source{};
    source.coreWindow = uwp_GetWindowReference();
    wgpu::SurfaceDescriptor surfaceDescriptor{};
    surfaceDescriptor.nextInChain = &source;
    auto surface = instance.CreateSurface(&surfaceDescriptor);
    if (!source.coreWindow || !surface) {
      fhUwpLog("Dawn", "CoreWindow surface creation failed");
      SDL_DestroyWindow(window);
      SDL_Quit();
      return 1;
    }
    wgpu::Adapter adapter;
    wgpu::RequestAdapterOptions options{};
    options.backendType = wgpu::BackendType::D3D12;
    options.powerPreference = wgpu::PowerPreference::HighPerformance;
    options.compatibleSurface = surface;
    auto request = instance.RequestAdapter(&options, wgpu::CallbackMode::WaitAnyOnly,
        [&](wgpu::RequestAdapterStatus status, wgpu::Adapter value, wgpu::StringView message) {
          if (status == wgpu::RequestAdapterStatus::Success) {
            adapter = std::move(value);
          } else {
            SDL_Log("[Dawn] %.*s", static_cast<int>(message.length), message.data);
          }
        });
    if (instance.WaitAny(request, 5000000000) != wgpu::WaitStatus::Success || !adapter) {
      fhUwpLog("Dawn", "Adapter request failed");
      SDL_DestroyWindow(window);
      SDL_Quit();
      return 1;
    }
    fhUwpLog("Dawn", "adapter acquired");
    wgpu::Device device;
    wgpu::DeviceDescriptor deviceDescriptor{};
    deviceDescriptor.SetUncapturedErrorCallback(
        [](const wgpu::Device&, wgpu::ErrorType, wgpu::StringView message) {
          SDL_Log("[Dawn] %.*s", static_cast<int>(message.length), message.data);
        });
    request = adapter.RequestDevice(&deviceDescriptor, wgpu::CallbackMode::WaitAnyOnly,
        [&](wgpu::RequestDeviceStatus status, wgpu::Device value, wgpu::StringView) {
          if (status == wgpu::RequestDeviceStatus::Success) {
            device = std::move(value);
          }
        });
    if (instance.WaitAny(request, 5000000000) != wgpu::WaitStatus::Success || !device) {
      fhUwpLog("D3D12", "Device creation failed");
      SDL_DestroyWindow(window);
      SDL_Quit();
      return 1;
    }
    fhUwpLog("D3D12", "device created");
    wgpu::SurfaceCapabilities capabilities{};
    if (surface.GetCapabilities(adapter, &capabilities) != wgpu::Status::Success || capabilities.formatCount == 0) {
      fhUwpLog("Dawn", "Surface capabilities unavailable");
      SDL_DestroyWindow(window);
      SDL_Quit();
      return 1;
    }
    wgpu::SurfaceConfiguration config{};
    config.device = device;
    config.format = capabilities.formats[0];
    config.usage = wgpu::TextureUsage::RenderAttachment;
    config.presentMode = wgpu::PresentMode::Fifo;
    config.alphaMode = wgpu::CompositeAlphaMode::Auto;
    bool running = true;
    bool presented = false;
    SDL_Gamepad* gamepad = nullptr;
    while (running) {
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
          running = false;
        } else if (event.type == SDL_EVENT_GAMEPAD_ADDED && gamepad == nullptr) {
          gamepad = SDL_OpenGamepad(event.gdevice.which);
          if (gamepad != nullptr) {
            fhUwpLog("PAD", "controller detected");
          }
        } else if (event.type == SDL_EVENT_GAMEPAD_REMOVED && gamepad != nullptr &&
                   SDL_GetGamepadID(gamepad) == event.gdevice.which) {
          SDL_CloseGamepad(gamepad);
          gamepad = nullptr;
        }
      }
      int width = 0;
      int height = 0;
      SDL_GetWindowSizeInPixels(window, &width, &height);
      if (!running || width <= 0 || height <= 0) {
        SDL_Delay(16);
        continue;
      }
      if (config.width != static_cast<uint32_t>(width) || config.height != static_cast<uint32_t>(height)) {
        config.width = width;
        config.height = height;
        surface.Configure(&config);
      }
      wgpu::SurfaceTexture frame{};
      surface.GetCurrentTexture(&frame);
      if (!frame.texture) {
        surface.Configure(&config);
        SDL_Delay(16);
        continue;
      }
      wgpu::RenderPassColorAttachment color{};
      color.view = frame.texture.CreateView();
      color.loadOp = wgpu::LoadOp::Clear;
      color.storeOp = wgpu::StoreOp::Store;
      color.clearValue = {0.04, 0.12, 0.25, 1.0};
      wgpu::RenderPassDescriptor passDescriptor{};
      passDescriptor.colorAttachmentCount = 1;
      passDescriptor.colorAttachments = &color;
      auto encoder = device.CreateCommandEncoder();
      auto pass = encoder.BeginRenderPass(&passDescriptor);
      pass.End();
      auto command = encoder.Finish();
      device.GetQueue().Submit(1, &command);
      surface.Present();
      if (!presented) {
        presented = true;
        fhUwpLog("D3D12", "First frame presented");
      }
      SDL_Delay(16);
    }
    if (gamepad != nullptr) {
      SDL_CloseGamepad(gamepad);
    }
    surface.Unconfigure();
    result = 0;
  }
  SDL_DestroyWindow(window);
  SDL_Quit();
  return result;
}
