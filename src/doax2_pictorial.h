#pragma once

#include <cstdint>

namespace doax2 {
// Only the native title/character selectors and the two scene-list builders
// may override availability. Never call the collection predicate to unlock:
// it may cache completion flags in the player's profile.
constexpr bool PictorialCharacterCaller(uint32_t character, uint32_t caller) {
  return character < 9 && (caller == 0x825A0110 || caller == 0x826EEA58);
}
constexpr bool PictorialClipCaller(uint32_t character, uint32_t category,
                                   uint32_t index, uint32_t count, uint32_t caller) {
  return character < 9 && category < 9 && count <= 256 && index < count &&
      (caller == 0x826EE050 || caller == 0x826EE468);
}
}  // namespace doax2
