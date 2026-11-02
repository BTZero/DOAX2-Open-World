#include "generated/default/doax2_pch.h"
#include "src/doax2_options.h"
#include "src/doax2_pictorial.h"

#include <windows.h>
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void Check(bool condition) {
  if (!condition) throw std::runtime_error("Options check failed");
}
}

int main() {
  for (uint32_t character = 0; character < 9; ++character) {
    Check(doax2::PictorialCharacterCaller(character, 0x825A0110));
    Check(doax2::PictorialCharacterCaller(character, 0x826EEA58));
    Check(!doax2::PictorialCharacterCaller(character, 0));
    for (uint32_t category = 0; category < 9; ++category) {
      Check(doax2::PictorialClipCaller(character, category, 20, 21, 0x826EE050));
      Check(doax2::PictorialClipCaller(character, category, 0, 21, 0x826EE468));
      Check(!doax2::PictorialClipCaller(character, category, 21, 21, 0x826EE050));
      Check(!doax2::PictorialClipCaller(character, category, 0, 0, 0x826EE050));
      Check(!doax2::PictorialClipCaller(character, category, 0, 257, 0x826EE050));
      Check(!doax2::PictorialClipCaller(character, category, 0, 21, 0));
    }
  }
  Check(!doax2::PictorialCharacterCaller(9, 0x825A0110));
  Check(!doax2::PictorialClipCaller(9, 0, 0, 21, 0x826EE050));
  Check(!doax2::PictorialClipCaller(0, 9, 0, 21, 0x826EE050));
  // Reserve the guest address range, committing only the four physics slots.
  auto* base = static_cast<uint8_t*>(VirtualAlloc(nullptr, size_t{1} << 32,
                                                 MEM_RESERVE, PAGE_NOACCESS));
  if (!base) return 1;
  const auto root = std::filesystem::current_path() /
      ("options-test-" + std::to_string(GetCurrentProcessId()));
  try {
    Check(VirtualAlloc(base + 0x833A1000, 0x2000, MEM_COMMIT, PAGE_READWRITE) != nullptr);
    std::filesystem::create_directories(root);
    const auto settings = root / "recomp-options.ini";
    constexpr std::array<uint32_t, 4> offsets{224, 232, 4, 116};
    constexpr std::array<float, 4> values{-0.75f, 0.5f, -0.0f, 0.25f};
    for (int percent : {100, 50, 0, -10, 150}) {
      { std::ofstream output(settings); output << "jiggle_strength=" << percent << '\n'; }
      Doax2LoadOptions(root);
      const float scale = (percent < 0 ? 0 : percent > 100 ? 100 : percent) / 100.0f;
      for (uint32_t slot = 0; slot < 4; ++slot) {
        const uint32_t source = 0x833A10D0 + 384 * slot;
        for (size_t i = 0; i < offsets.size(); ++i)
          REX_STORE_U32(source + offsets[i], std::bit_cast<uint32_t>(values[i]));
        {
          Doax2JiggleInputs inputs(base, slot);
          for (size_t i = 0; i < offsets.size(); ++i)
            Check(REX_LOAD_U32(source + offsets[i]) == std::bit_cast<uint32_t>(values[i] * scale));
        }
        for (size_t i = 0; i < offsets.size(); ++i)
          Check(REX_LOAD_U32(source + offsets[i]) == std::bit_cast<uint32_t>(values[i]));
      }
    }
    // An invalid input must leave every field untouched, including earlier ones.
    { std::ofstream output(settings); output << "jiggle_strength=50\n"; }
    Doax2LoadOptions(root);
    const uint32_t source = 0x833A10D0;
    REX_STORE_U32(source + 116, 0x7FC00000);
    { Doax2JiggleInputs inputs(base, 0);
      Check(REX_LOAD_U32(source + 224) == std::bit_cast<uint32_t>(values[0]));
      Check(REX_LOAD_U32(source + 116) == 0x7FC00000); }
    { Doax2JiggleInputs invalid_slot(base, 4); }
    // Loading a missing or malformed file resets a previously reduced setting.
    for (const auto& [contents, expected] : std::array<std::pair<const char*, float>, 4>{{
        {"jiggle_strength=0\n", -0.0f}, {"jiggle_strength=bad\n", values[0]},
        {"jiggle_strength=50junk\n", values[0]}, {"", values[0]}}}) {
      { std::ofstream output(settings); output << contents; }
      Doax2LoadOptions(root);
      REX_STORE_U32(source + 116, std::bit_cast<uint32_t>(values[3]));
      { Doax2JiggleInputs inputs(base, 0);
        Check(REX_LOAD_U32(source + 224) == std::bit_cast<uint32_t>(expected)); }
    }
    std::filesystem::remove(settings);
    Doax2LoadOptions(root);
    { Doax2JiggleInputs inputs(base, 0);
      Check(REX_LOAD_U32(source + 224) == std::bit_cast<uint32_t>(values[0])); }
    std::filesystem::remove(root);
    VirtualFree(base, 0, MEM_RELEASE);
    std::cout << "Options checks passed\n";
    return 0;
  } catch (const std::exception& error) {
    VirtualFree(base, 0, MEM_RELEASE);
    std::cerr << error.what() << '\n';
    return 1;
  }
}
