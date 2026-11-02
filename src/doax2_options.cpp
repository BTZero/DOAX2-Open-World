#include "generated/default/doax2_pch.h"
#include "doax2_options.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <fstream>
#include <string>
#include <string_view>

#include <imgui.h>
#include <rex/logging.h>
#include <rex/ui/imgui_dialog.h>

namespace {
std::atomic<int> jiggle_percent{100};
std::atomic<bool> options_visible{false};
std::filesystem::path settings_path;
constexpr std::array<uint32_t, 4> kInputs{224, 232, 4, 116};

void SaveStrength(int percent) {
  jiggle_percent.store(percent, std::memory_order_relaxed);
  std::error_code error;
  std::filesystem::create_directories(settings_path.parent_path(), error);
  if (error) {
    REXLOG_WARN("DOAX2 Options: cannot create settings directory: {}", error.message());
    return;
  }
  std::ofstream output(settings_path, std::ios::trunc);
  output << "jiggle_strength=" << percent << '\n';
  output.close();
  if (!output) REXLOG_WARN("DOAX2 Options: could not save jiggle strength");
}

class OptionsDialog final : public rex::ui::ImGuiDialog {
 public:
  explicit OptionsDialog(rex::ui::ImGuiDrawer* drawer) : ImGuiDialog(drawer) {}
 protected:
  void OnDraw(ImGuiIO& io) override {
    if (!options_visible.load(std::memory_order_relaxed)) return;
    // Extend the original Options screen without replacing its native menu,
    // input routing or saved profile. Stay clear of its left-hand list.
    const float scale = std::max(0.65f, std::min(io.DisplaySize.x / 1280.0f,
                                              io.DisplaySize.y / 720.0f));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 24 * scale, 180 * scale),
                           ImGuiCond_Always, ImVec2(1, 0));
    ImGui::SetNextWindowSize(ImVec2(340 * scale, 0), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.94f);
    if (ImGui::Begin("Jiggle Physics", nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoFocusOnAppearing)) {
      int percent = jiggle_percent.load(std::memory_order_relaxed);
      ImGui::TextWrapped("Reduce secondary body motion.");
      ImGui::SetNextItemWidth(-1);
      if (ImGui::SliderInt("##jiggle_strength", &percent, 0, 100, "%d%%",
                           ImGuiSliderFlags_AlwaysClamp)) {
        SaveStrength(percent);
      }
      ImGui::TextWrapped("0%%: off   100%%: original");
      ImGui::TextDisabled("Saved automatically. Use the mouse to adjust.");
    }
    ImGui::End();
  }
};
}  // namespace

void Doax2LoadOptions(const std::filesystem::path& user_root) {
  jiggle_percent.store(100, std::memory_order_relaxed);
  options_visible.store(false, std::memory_order_relaxed);
  settings_path = user_root / "recomp-options.ini";
  std::ifstream input(settings_path);
  std::string line;
  while (std::getline(input, line)) {
    constexpr std::string_view key = "jiggle_strength=";
    if (!line.starts_with(key)) continue;
    try {
      size_t end = 0;
      const auto value = line.substr(key.size());
      const int percent = std::stoi(value, &end);
      if (end == value.size()) jiggle_percent.store(std::clamp(percent, 0, 100));
    } catch (...) { /* Keep the original strength for malformed settings. */ }
  }
  REXLOG_INFO("DOAX2 Options: jiggle strength {}%", jiggle_percent.load());
}

void Doax2ObserveOptions(uint8_t* base) {
  constexpr uint32_t manager = 0x834BCFE0;
  // Title table, screen 6 = Options, descriptor update 82591298. The
  // navigation depth must be zero so audio/language/video pages stay clear.
  options_visible.store(REX_LOAD_U8(manager) == 6 &&
      REX_LOAD_U32(manager + 68) == 0x82B83C80 &&
      REX_LOAD_U32(manager + 40) == 1 &&
      REX_LOAD_U16(0x83984F4C) == 0, std::memory_order_relaxed);
}

std::unique_ptr<rex::ui::ImGuiDialog> Doax2CreateOptionsDialog(rex::ui::ImGuiDrawer* drawer) {
  return std::make_unique<OptionsDialog>(drawer);
}

Doax2JiggleInputs::Doax2JiggleInputs(uint8_t* base, uint32_t slot) : base_(base) {
  const int percent = jiggle_percent.load(std::memory_order_relaxed);
  if (percent == 100 || slot >= 4) return;
  const uint32_t source = 0x833A10D0 + 384 * slot;
  std::array<float, 4> values;
  for (size_t i = 0; i < kInputs.size(); ++i) {
    saved_[i] = REX_LOAD_U32(source + kInputs[i]);
    values[i] = std::bit_cast<float>(saved_[i]);
    if (!std::isfinite(values[i])) return;
  }
  source_ = source;
  for (size_t i = 0; i < kInputs.size(); ++i) {
    REX_STORE_U32(source + kInputs[i], std::bit_cast<uint32_t>(values[i] * (percent / 100.0f)));
  }
}

Doax2JiggleInputs::~Doax2JiggleInputs() {
  if (!source_) return;
  uint8_t* base = base_;
  for (size_t i = 0; i < kInputs.size(); ++i) REX_STORE_U32(source_ + kInputs[i], saved_[i]);
}
