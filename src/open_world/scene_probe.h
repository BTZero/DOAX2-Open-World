#pragma once

#include <cstdint>

// Observations only. These functions never change guest memory or PPC registers.
// Call on the guest update thread; unavailable in ordinary builds.
namespace doax2::open_world {
bool OverridePictorialUnlock(uint32_t character, uint32_t caller);
struct ScriptCommandScope {
  ScriptCommandScope(const uint8_t* base, uint32_t state, uint32_t handler);
  ~ScriptCommandScope();
  ScriptCommandScope(const ScriptCommandScope&) = delete;
  ScriptCommandScope& operator=(const ScriptCommandScope&) = delete;
 private:
  const uint8_t* base_{};
  uint32_t handler_{}, pointer_{}, words_{}, before_{}, camera_before_{};
  uint16_t header_{}, arguments_[3]{};
  uint8_t mapping_[4]{};
};
void ObserveScreen(const uint8_t* base, const char* boundary);
void ObserveScheduler(const uint8_t* base);
void ObserveActivity(uint32_t function, uint32_t argument);
void ObserveFrame(const uint8_t* base);
void ObserveRig(const uint8_t* base, uint32_t slot);
}  // namespace doax2::open_world
