/*
 * CrossDesk feature/parity - game key mapping engine.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef CROSSDESK_KEY_MAP_H_
#define CROSSDESK_KEY_MAP_H_

#include <cstdint>
#include <string>
#include <vector>

namespace crossdesk {
namespace keymap {

enum class SourceType : uint8_t {
  key = 0,
  gamepad_button = 1,
  gamepad_axis = 2,
  touch_zone = 3,
  mouse_button = 4,
};

enum class ActionType : uint8_t {
  key = 0,
  key_chord = 1,
  mouse_click = 2,
  mouse_move = 3,
  wheel = 4,
  macro = 5,
};

// An input event produced by a controller. For touch_zone rules, x/y are the
// touch point and w/h are ignored; for stored touch_zone rules, x/y/w/h are
// normalized [0,1] rectangle bounds.
struct Source {
  SourceType type = SourceType::key;
  uint32_t code = 0;
  int threshold = 0;
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  float h = 0.0f;
};

// An output action understood by the existing remote input pipeline.
// keys are Windows virtual-key codes, matching libs/wire Key.key_value.
struct Target {
  ActionType type = ActionType::key;
  std::vector<uint32_t> keys;
  int mouse_button = 0;  // 0 left, 1 right, 2 middle
  float dx = 0.0f;
  float dy = 0.0f;
  uint32_t delay_ms = 0;
};

struct Rule {
  std::string id;
  Source source;
  Target target;
  bool enabled = true;
};

struct Profile {
  std::string name;
  std::vector<Rule> rules;
};

class KeyMapEngine {
 public:
  bool LoadProfile(const Profile& profile);
  const Profile& profile() const { return profile_; }
  void Clear();

  // value is only used by gamepad_axis. For touch_zone, source.x/y is tested
  // against each rule's normalized rectangle.
  bool Resolve(const Source& source, int value,
               std::vector<Target>& targets) const;

  // Stable line format, safe to store in ConfigCenter / SharedPreferences /
  // UserDefaults as a single string.
  std::string Serialize() const;
  bool Deserialize(const std::string& text);

 private:
  Profile profile_;
};

std::string SourceTypeName(SourceType type);
SourceType ParseSourceType(const std::string& name);
std::string ActionTypeName(ActionType type);
ActionType ParseActionType(const std::string& name);

bool InsideTouchZone(const Source& zone, float x, float y);
Profile DefaultFpsProfile();
Profile DefaultMobaProfile();

}  // namespace keymap
}  // namespace crossdesk

#endif  // CROSSDESK_KEY_MAP_H_
