#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>

namespace rex::ui { class ImGuiDrawer; class ImGuiDialog; }

void Doax2LoadOptions(const std::filesystem::path& user_root);
void Doax2ObserveOptions(uint8_t* base);
std::unique_ptr<rex::ui::ImGuiDialog> Doax2CreateOptionsDialog(rex::ui::ImGuiDrawer* drawer);

// Scale only the inputs consumed by the native deformation publisher. The
// integrator's displacement, velocity and interpolation history stay intact.
class Doax2JiggleInputs {
 public:
  Doax2JiggleInputs(uint8_t* base, uint32_t slot);
  ~Doax2JiggleInputs();
  Doax2JiggleInputs(const Doax2JiggleInputs&) = delete;
  Doax2JiggleInputs& operator=(const Doax2JiggleInputs&) = delete;
 private:
  uint8_t* base_;
  uint32_t source_ = 0;
  std::array<uint32_t, 4> saved_{};
};
