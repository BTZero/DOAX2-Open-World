#pragma once

#include <cstdint>
#include <array>
struct PPCContext;

// Call on the guest update thread; unavailable in ordinary builds.
// Observe functions are read-only. The opt-in availability helpers override
// predicate results, and ScriptCommandScope only observes native commands.
namespace doax2::open_world {
void MaintainIdleBank(uint8_t* base, PPCContext& ctx);
void HoldIdleRoot(uint8_t* base, PPCContext& ctx, uint32_t slot);
void CommitIdleMovement(uint8_t* base, uint32_t slot);
struct IdleUpdateScope {
  IdleUpdateScope(uint8_t* base, uint32_t slot);
  ~IdleUpdateScope();
 private:
  uint8_t* base_{};
};
struct IdleCommandScope {
  IdleCommandScope(uint8_t* base, PPCContext& ctx);
  ~IdleCommandScope();
 private:
  uint8_t* base_{};
  PPCContext* ctx_{};
};
struct VolleyballMotionScope {
  VolleyballMotionScope(const uint8_t* base, uint32_t player, uint32_t motion,
                        double rate, uint32_t caller);
  ~VolleyballMotionScope();
 private:
  const uint8_t* base_{};
  uint32_t player_{}, motion_{}, caller_{}, before_{};
  double rate_{};
};
struct RootMotionScope {
  RootMotionScope(const uint8_t* base, uint32_t slot);
  ~RootMotionScope();
 private:
  const uint8_t* base_{};
  uint32_t slot_{}, motion_{};
  std::array<float, 3> before_{}, delta_{};
};
struct OrbitCameraScope {
  OrbitCameraScope(uint8_t* base, uint32_t object);
  ~OrbitCameraScope();
  OrbitCameraScope(const OrbitCameraScope&) = delete;
  OrbitCameraScope& operator=(const OrbitCameraScope&) = delete;
 private:
  uint8_t* base_{};
  uint32_t object_{};
  std::array<uint32_t, 16> pose_{};
};
struct ActorClockHoldScope {
  ActorClockHoldScope(uint8_t* base, uint32_t slot, uint32_t track);
  ~ActorClockHoldScope();
  ActorClockHoldScope(const ActorClockHoldScope&) = delete;
  ActorClockHoldScope& operator=(const ActorClockHoldScope&) = delete;
 private:
  uint8_t* base_{};
  uint32_t object_{};
  std::array<uint32_t, 4> clocks_{};
  std::array<uint32_t, 6> identity_{};
};
bool OverridePictorialUnlock(uint32_t character, uint32_t caller);
void OverridePictorialClip(const uint8_t* base, PPCContext& ctx,
                          uint32_t character, uint32_t category,
                          uint32_t index, uint32_t caller);
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
struct CameraScriptScope {
  CameraScriptScope(const uint8_t* base, uint32_t object);
  ~CameraScriptScope();
  CameraScriptScope(const CameraScriptScope&) = delete;
  CameraScriptScope& operator=(const CameraScriptScope&) = delete;
 private:
  const uint8_t* base_{};
  uint32_t object_{}, phase_{}, entry_time_{}, entry_type_{};
  std::array<uint32_t, 10> before_{};
};
struct CameraClockHoldScope {
  CameraClockHoldScope(uint8_t* base, uint32_t object);
  ~CameraClockHoldScope();
  CameraClockHoldScope(const CameraClockHoldScope&) = delete;
  CameraClockHoldScope& operator=(const CameraClockHoldScope&) = delete;
 private:
  uint8_t* base_{};
  uint32_t object_{};
  std::array<uint32_t, 12> clocks_{};
  std::array<uint32_t, 11> identity_{};
};
void ObserveScreen(const uint8_t* base, const char* boundary);
void ObserveScheduler(const uint8_t* base);
void ObserveActivity(uint32_t function, uint32_t argument);
void ObserveFrame(const uint8_t* base);
void ObserveRig(const uint8_t* base, uint32_t slot);
void ObservePictorialMotion(const uint8_t* base, uint32_t slot, bool after);
}  // namespace doax2::open_world
