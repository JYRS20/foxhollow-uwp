#include "menu_view.hpp"
#include "ui_text.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

namespace {
constexpr ImU32 white = IM_COL32(244, 244, 240, 255);
constexpr ImU32 orange = IM_COL32(255, 111, 15, 255);

void importProgress(const FhMenuModel& model, float width, float scale) {
  const float fraction = model.totalBytes ? std::clamp(static_cast<float>(
      static_cast<double>(model.copiedBytes) / model.totalBytes), 0.f, 1.f) : 0.f;
  char label[64];
  std::snprintf(label, sizeof(label), "%d%%", model.verifying ? 100 : std::min(99, static_cast<int>(fraction * 100)));
  ImGui::TextUnformatted(fhUiText(!model.importPhase.empty() ? fhUiText(model.importPhase.c_str()) : model.verifying ? "Verificando el disco..." : "Copiando desde USB..."));
  ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.f, .44f, .06f, 1));
  ImGui::ProgressBar(fraction, ImVec2(width, 30 * scale), label);
  ImGui::PopStyleColor();
  if (model.totalBytes)
    ImGui::TextDisabled(fhUiText("%.1f / %.1f MB"), model.copiedBytes / 1048576.0, model.totalBytes / 1048576.0);
}

void background(ImVec2 size) {
  ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), size, IM_COL32(0, 0, 0, 255));
}

bool choice(const char* text, float width, float height, bool disabled = false, bool focus = false, const char* id = nullptr) {
  ImGui::PushID(id ? id : text);
  text = fhUiText(text);
  ImGui::BeginDisabled(disabled);
  if (focus && !disabled) ImGui::SetKeyboardFocusHere();
  const bool activated = ImGui::Selectable("##choice", false, 0, ImVec2(width, height));
  const auto min = ImGui::GetItemRectMin();
  const auto max = ImGui::GetItemRectMax();
  const bool selected = !disabled && (ImGui::IsItemFocused() || ImGui::IsItemHovered());
  auto* draw = ImGui::GetWindowDrawList();
  if (selected) {
    draw->AddRectFilled(min, max, IM_COL32(244, 233, 207, 255), 4.f);
    draw->AddRectFilled(min, ImVec2(min.x + 4, max.y), orange, 4.f);
  }
  const auto textSize = ImGui::CalcTextSize(text);
  draw->AddText(ImVec2(min.x + 18, min.y + (height - textSize.y) * .5f),
      disabled ? IM_COL32(112, 124, 131, 255) : selected ? IM_COL32(22, 30, 34, 255) : white, text);
  ImGui::EndDisabled();
  ImGui::PopID();
  return activated;
}
void settingsControls(FhMenuModel& model, float scale) {
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::TextDisabled("%s", fhUiText("IDIOMA DEL MENÚ"));
      constexpr std::array interfaceNames{"Español", "English", "Français", "Deutsch", "Italiano"};
      ImGui::SetNextItemWidth(520 * scale);
      if (ImGui::Combo("##interface-language", &model.settings.uiLanguage, interfaceNames.data(), 5))
        fhUiSetLanguage(model.settings.uiLanguage);
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::TextDisabled("%s", fhUiText("GRÁFICOS"));
      ImGui::TextUnformatted(fhUiText("Relación de aspecto"));
      if (model.focus) ImGui::SetKeyboardFocusHere();
      ImGui::RadioButton(fhUiLabel("4:3 (original)").c_str(), &model.settings.widescreen, 0);
      ImGui::SameLine(300 * scale);
      ImGui::RadioButton(fhUiLabel("16:9 (panorámico)").c_str(), &model.settings.widescreen, 1);
      ImGui::Spacing();
      ImGui::TextUnformatted(fhUiText("Resolución interna"));
      ImGui::RadioButton(fhUiLabel("720p").c_str(), &model.settings.renderHeight, 720);
      ImGui::SameLine(300 * scale);
      ImGui::RadioButton(fhUiLabel("1080p").c_str(), &model.settings.renderHeight, 1080);
      bool showFps = model.settings.showFps != 0;
      if (ImGui::Checkbox(fhUiLabel("Contador de FPS").c_str(), &showFps)) model.settings.showFps = showFps ? 1 : 0;
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();
      ImGui::TextDisabled("%s", fhUiText("IDIOMA DEL JUEGO"));
      constexpr std::array ids{-1, 0, 1, 2, 3, 5};
      const std::array names{fhUiText("Automático"), fhUiText("Inglés"), fhUiText("Francés"), fhUiText("Alemán"), fhUiText("Italiano"), fhUiText("Español")};
      int selected = 0;
      for (int i = 0; i < static_cast<int>(ids.size()); ++i) if (model.settings.language == ids[i]) selected = i;
      ImGui::BeginDisabled(!model.pal);
      ImGui::SetNextItemWidth(520 * scale);
      if (ImGui::Combo("##language", &selected, names.data(), static_cast<int>(names.size())))
        model.settings.language = ids[selected];
      ImGui::EndDisabled();
      if (!model.pal) ImGui::TextDisabled("%s", fhUiText("Selecciona primero un disco PAL."));

}
void fileControls(FhMenuModel& model, float scale, FhMenuCommand& command) {
      if (model.textureMode && !model.importing) {
        ImGui::TextWrapped(fhUiText("%s"), model.textureCount ? model.textureName.c_str() : fhUiText("No hay un pack instalado."));
        if (model.textureCount) {
          ImGui::TextDisabled(fhUiText("%u texturas / %u omitidas"), model.textureCount, model.textureSkipped);
          if (choice(model.texturesEnabled ? "DESACTIVAR TEXTURAS" : "ACTIVAR TEXTURAS", 480 * scale, 40 * scale, model.busy, false, "texture-toggle"))
            command.action = FhMenuAction::ToggleTextures;
        }
      } else if (model.ready && !model.textureMode) ImGui::TextWrapped(fhUiText("%s"), model.disc.c_str());
      if (!model.busy && model.error.empty() && model.totalBytes && model.copiedBytes == model.totalBytes)
        ImGui::TextColored(ImVec4(.5f, .82f, .64f, 1.f), "%s", fhUiText(model.textureMode ? "Pack importado correctamente." : "Importacion completada. Disco listo."));
      ImGui::SetWindowFontScale(1.35f / 2.1f);
      ImGui::TextWrapped(fhUiText("%s"), fhUiText(model.location.c_str()));
      ImGui::SetWindowFontScale(1.65f / 2.1f);
      if (model.importing) {
        ImGui::TextWrapped(fhUiText("%s"), model.importName.c_str());
        importProgress(model, 620 * scale, scale);
        if (model.textureMode) ImGui::TextDisabled("%s", fhUiText("Juego y audio pausados durante la preparacion."));
        ImGui::TextWrapped("%s", fhUiText("No retires la USB hasta finalizar."));
        if (choice("Cancelar importacion", 480 * scale, 48 * scale)) command.action = FhMenuAction::Cancel;
      } else {
        ImGui::BeginDisabled(model.busy);
        if (model.canGoBack) {
          if (ImGui::Button(fhUiLabel("Subir").c_str())) command.action = FhMenuAction::Back;
          ImGui::SameLine();
        }
        if (model.focus) ImGui::SetKeyboardFocusHere();
        if (ImGui::Button(fhUiLabel("Actualizar").c_str())) command.action = FhMenuAction::Refresh;
        ImGui::BeginChild("Files", ImVec2(0, 0), ImGuiChildFlags_NavFlattened);
        ImGui::SetWindowFontScale(1.f);
        if (model.files.empty() && !model.busy)
          ImGui::TextWrapped("%s", fhUiText(model.canGoBack ? model.textureMode ? "No hay packs ZIP en esta carpeta." : "No hay discos ISO, RVZ o GCM en esta carpeta."
              : "No se detectaron dispositivos USB. Conecta la USB y pulsa Actualizar."));
        for (int i = 0; i < static_cast<int>(model.files.size()); ++i) {
          ImGui::PushID(i);
          const auto label = (model.files[i].folder ? fhUiText("[Carpeta] ") : model.textureMode ? "[ZIP] " : fhUiText("[Disco] ")) + model.files[i].name;
          if (ImGui::Selectable(label.c_str(), false, 0, ImVec2(0, 44 * scale))) command = {FhMenuAction::Open, i};
          ImGui::PopID();
        }
        ImGui::EndChild();
        ImGui::EndDisabled();
      }
}
FhMenuCommand gameToolbar(FhMenuModel& model) {
  FhMenuCommand command;
  const auto size = ImGui::GetIO().DisplaySize;
  const float scale = std::min(size.x / 1280.f, size.y / 720.f);
  const float fontUnit = 13.f / ImGui::GetFont()->FontSize;
  const auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12 * scale, 8 * scale));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12 * scale, 10 * scale));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(.025f, .025f, .025f, .88f));
  ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(.09f, .15f, .19f, 1));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(.15f, .24f, .29f, 1));
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(.21f, .30f, .35f, 1));
  ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.f, .44f, .06f, 1));
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(size.x, 60 * scale));
  if (model.focus && model.page == 0) ImGui::SetNextWindowFocus();
  ImGui::Begin("FoxhollowToolbar", nullptr, flags);
  ImGui::SetWindowFontScale(1.55f * scale * fontUnit);
  if (choice("CONFIGURACIÓN", 190 * scale, 44 * scale, model.busy, model.focus && model.page == 0))
    command.action = FhMenuAction::Settings;
  ImGui::SameLine();
  if (choice("TEXTURAS", 150 * scale, 44 * scale, model.busy)) command.action = FhMenuAction::Textures;
  ImGui::SameLine();
  if (choice("SALIR", 104 * scale, 44 * scale)) command.action = FhMenuAction::ExitMenu;
  ImGui::SameLine(size.x - 60 * scale);
  if (choice("X", 40 * scale, 44 * scale, model.busy)) command.action = FhMenuAction::Resume;
  ImGui::End();
  if (model.page == 1 || model.page == 3 || model.page == 5) {
    ImGui::SetNextWindowPos(ImVec2(24 * scale, 80 * scale));
    ImGui::SetNextWindowSize(ImVec2(680 * scale, model.page != 1 ? 360 * scale : size.y - 112 * scale));
    if (model.focus) ImGui::SetNextWindowFocus();
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(.02f, .025f, .03f, .94f));
    ImGui::Begin("FoxhollowPanel", nullptr, flags);
    ImGui::SetWindowFontScale(1.7f * scale * fontUnit);
    ImGui::TextUnformatted(fhUiText(model.page == 1 ? "CONFIGURACIÓN" : model.page == 5 ? "TEXTURAS" : "SALIR"));
    ImGui::Separator();
    ImGui::BeginChild("Content", ImVec2(0, -104 * scale), ImGuiChildFlags_NavFlattened);
    ImGui::SetWindowFontScale(1.f);
    if (model.page == 1) settingsControls(model, scale);
    else if (model.page == 5) {
      ImGui::TextWrapped(fhUiText("%s"), model.textureCount ? model.textureName.c_str() : fhUiText("No hay un pack instalado."));
      if (model.textureCount) {
        ImGui::TextDisabled(fhUiText("%u texturas / %u omitidas"), model.textureCount, model.textureSkipped);
        ImGui::TextUnformatted(fhUiText(model.texturesEnabled ? "Estado: activadas" : "Estado: desactivadas"));
        if (choice(model.texturesEnabled ? "DESACTIVAR TEXTURAS" : "ACTIVAR TEXTURAS",
            480 * scale, 44 * scale, model.busy, model.focus, "texture-toggle")) command.action = FhMenuAction::ToggleTextures;
      } else ImGui::TextWrapped("%s", fhUiText("Instala el ZIP desde TEXTURAS en el menu principal."));
    }
    else {
      if (choice("REINICIAR", 480 * scale, 44 * scale, model.busy, model.focus))
        command.action = FhMenuAction::Restart;
      if (choice("SALIR DE LA APLICACIÓN", 480 * scale, 44 * scale, model.busy))
        command.action = FhMenuAction::Quit;
    }
    ImGui::EndChild();
    if (choice(model.page == 1 ? "GUARDAR Y VOLVER" : "VOLVER", 480 * scale, 44 * scale, model.busy))
      command.action = model.page == 1 ? FhMenuAction::Save : FhMenuAction::Home;
    ImGui::TextDisabled("%s", fhUiText("B: volver    View: cerrar    A: seleccionar"));
    ImGui::End();
    ImGui::PopStyleColor();
  } else if (model.restart) {
    ImGui::GetForegroundDrawList()->AddText(ImVec2(24 * scale, 68 * scale), orange,
        fhUiText("Cambios pendientes: reinicia para aplicarlos."));
  }
  if (!model.error.empty())
    ImGui::GetForegroundDrawList()->AddText(ImVec2(24 * scale, size.y - 30 * scale), orange, fhUiText(model.error.c_str()));
  ImGui::PopStyleColor(8);
  ImGui::PopStyleVar(2);
  model.focus = false;
  return command;
}

}

FhMenuCommand fhMenuDraw(FhMenuModel& model, ImTextureID logo) {
  fhUiSetLanguage(model.settings.uiLanguage);
  ImGui::GetIO().ConfigNavCursorVisibleAlways = true;
  if (model.inGame) return gameToolbar(model);
  FhMenuCommand command;
  const auto size = ImGui::GetIO().DisplaySize;
  const float scale = std::min(size.x / 1280.f, size.y / 720.f);
  const float left = 72.f * scale;
  const float fontUnit = 13.f / ImGui::GetFont()->FontSize;
  if (!model.gameVisible) background(size);
  else ImGui::GetBackgroundDrawList()->AddRectFilledMultiColor(ImVec2(0, 0), size,
      IM_COL32(0, 0, 0, 165), IM_COL32(0, 0, 0, 0),
      IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 165));
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(size);
  if (model.focus && !model.busy) ImGui::SetNextWindowFocus();
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14 * scale, 10 * scale));
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16 * scale, 10 * scale));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(.09f, .15f, .19f, 1));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(.15f, .24f, .29f, 1));
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(.21f, .30f, .35f, 1));
  ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.f, .44f, .06f, 1));
  ImGui::Begin("Foxhollow", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);
  ImGui::SetWindowFontScale(2.1f * scale * fontUnit);
  ImGui::SetCursorPos(ImVec2(left, 72 * scale));
  if (logo) {
    const float width = (model.page == 0 ? 608.f : 456.f) * scale;
    ImGui::Image(logo, ImVec2(width, width * 112.f / 632.f));
  } else {
    ImGui::TextUnformatted(fhUiText("FOXHOLLOW"));
  }
  if (model.page == 0) {
    ImGui::SetCursorPos(ImVec2(left, 250 * scale));
    if (choice(model.restart ? "REINICIAR" : "JUGAR", 480 * scale, 56 * scale, model.busy || !model.ready, model.focus && model.ready))
      command.action = FhMenuAction::Play;
    ImGui::SetCursorPosX(left);
    if (choice("SELECCIONAR JUEGO", 480 * scale, 56 * scale, model.busy, model.focus && !model.ready))
      command.action = FhMenuAction::Browse;
    ImGui::SetCursorPosX(left);
    if (choice("TEXTURAS", 480 * scale, 56 * scale, model.busy)) command.action = FhMenuAction::Textures;
    ImGui::SetCursorPosX(left);
    if (choice("CONFIGURACIÓN", 480 * scale, 56 * scale)) command.action = FhMenuAction::Settings;
    ImGui::SetCursorPosX(left);
    if (choice("SALIR", 480 * scale, 56 * scale)) command.action = FhMenuAction::Quit;
    ImGui::SetWindowFontScale(1.5f * scale * fontUnit);
    ImGui::SetCursorPos(ImVec2(left + 22 * scale, size.y - 132 * scale));
    const auto status = model.importing ? "Importando juego desde USB..." : model.busy ? "Buscando archivos..." : model.ready ? "Disco listo" : "Selecciona tu ISO, RVZ o GCM desde una USB.";
    const auto position = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(position.x - 14 * scale, position.y + 9 * scale),
        4 * scale, model.ready ? IM_COL32(128, 208, 163, 255) : orange);
    ImGui::TextUnformatted(fhUiText(status));
    ImGui::SetCursorPosX(left + 22 * scale);
    ImGui::TextDisabled(fhUiText("GameCube  /  %s"), model.ready ? model.region.c_str() : "USB");
  } else {
    ImGui::SetCursorPos(ImVec2(left, 192 * scale));
    ImGui::TextUnformatted(fhUiText(model.page == 1 ? "CONFIGURACIÓN" : model.page == 4 ? "TEXTURAS" : "SELECCIONAR JUEGO"));
    ImGui::SetCursorPos(ImVec2(left, 240 * scale));
    ImGui::BeginChild("Panel", ImVec2(std::min(940.f * scale, size.x - 2 * left), size.y - 340 * scale),
        ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_NoSavedSettings);
    ImGui::SetWindowFontScale(1.65f / 2.1f);
    if (model.page == 1) {
      settingsControls(model, scale);
    } else {
      fileControls(model, scale, command);
    }
    ImGui::EndChild();
    ImGui::SetCursorPos(ImVec2(left, size.y - 88 * scale));
    if (choice(model.page == 1 ? "GUARDAR Y VOLVER" : "VOLVER", 480 * scale, 48 * scale, model.busy))
      command.action = model.page == 1 ? FhMenuAction::Save : FhMenuAction::Home;
  }
  ImGui::SetWindowFontScale(1.25f * scale * fontUnit);
  if (!model.error.empty()) {
    ImGui::SetCursorPos(ImVec2(size.x * .55f, size.y * .74f));
    ImGui::PushTextWrapPos(size.x - left);
    ImGui::TextColored(ImVec4(1, .6f, .3f, 1), "%s", fhUiText(model.error.c_str()));
    ImGui::PopTextWrapPos();
  }
  ImGui::SetCursorPos(ImVec2(left, size.y - 31 * scale));
  ImGui::TextDisabled("%s", fhUiText("A: seleccionar     B: volver     Cruceta / stick: navegar"));
  const char* version = "0.5.8.17";
  ImGui::SetCursorPos(ImVec2(size.x - left - ImGui::CalcTextSize(version).x, size.y - 31 * scale));
  ImGui::TextDisabled(fhUiText("%s"), version);
  ImGui::End();
  ImGui::PopStyleColor(8);
  ImGui::PopStyleVar(3);
  model.focus = model.busy && model.page == 0;
  return command;
}

void fhMenuDrawPerformance(double fps, double averageMs, double peakMs, bool toolbarOpen) {
  const auto size = ImGui::GetIO().DisplaySize;
  const float scale = std::min(size.x / 1280.f, size.y / 720.f);
  const float top = toolbarOpen ? 80.f : 20.f;
  const ImVec2 min(size.x - 280 * scale, top * scale);
  const ImVec2 max(size.x - 20 * scale, (top + 92) * scale);
  auto* draw = ImGui::GetForegroundDrawList();
  draw->AddRectFilled(min, max, IM_COL32(0, 0, 0, 210), 5 * scale);
  draw->AddRect(min, max, IM_COL32(255, 111, 15, 180), 5 * scale);
  char lines[3][64];
  if (fps > 0) {
    std::snprintf(lines[0], sizeof(lines[0]), "FPS: %.1f", fps);
    std::snprintf(lines[1], sizeof(lines[1]), fhUiText("Fotograma: %.1f ms"), averageMs);
    std::snprintf(lines[2], sizeof(lines[2]), fhUiText("Pico: %.1f ms"), peakMs);
  } else {
    std::snprintf(lines[0], sizeof(lines[0]), "FPS: --");
    std::snprintf(lines[1], sizeof(lines[1]), "%s", fhUiText("Fotograma: -- ms"));
    std::snprintf(lines[2], sizeof(lines[2]), "%s", fhUiText("Pico: -- ms"));
  }
  for (int i = 0; i < 3; ++i)
    draw->AddText(ImGui::GetFont(), 22 * scale,
        ImVec2(min.x + 12 * scale, min.y + (8 + i * 26) * scale), white, lines[i]);
}
