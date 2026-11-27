/*
 * CrossDesk feature/parity - game key mapping engine.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "key_map.h"

#include <algorithm>
#include <sstream>
#include <utility>

namespace crossdesk {
namespace keymap {
namespace {

std::vector<std::string> Split(const std::string& text, char delimiter) {
  std::vector<std::string> parts;
  std::string part;
  std::istringstream stream(text);
  while (std::getline(stream, part, delimiter)) {
    parts.push_back(part);
  }
  return parts;
}

bool ParseUInt(const std::string& text, uint32_t& value) {
  if (text.empty()) return false;
  try {
    std::size_t used = 0;
    unsigned long parsed = std::stoul(text, &used, 10);
    if (used != text.size()) return false;
    value = static_cast<uint32_t>(parsed);
    return true;
  } catch (...) {
    return false;
  }
}

bool ParseInt(const std::string& text, int& value) {
  if (text.empty()) return false;
  try {
    std::size_t used = 0;
    int parsed = std::stoi(text, &used, 10);
    if (used != text.size()) return false;
    value = parsed;
    return true;
  } catch (...) {
    return false;
  }
}

bool ParseFloat(const std::string& text, float& value) {
  if (text.empty()) return false;
  try {
    std::size_t used = 0;
    float parsed = std::stof(text, &used);
    if (used != text.size()) return false;
    value = parsed;
    return true;
  } catch (...) {
    return false;
  }
}

bool ParseKeys(const std::string& text, std::vector<uint32_t>& keys) {
  keys.clear();
  for (const auto& item : Split(text, ',')) {
    uint32_t key = 0;
    if (!ParseUInt(item, key)) return false;
    keys.push_back(key);
  }
  return !keys.empty();
}

bool IsKnownSourceType(const std::string& name) {
  return name == "key" || name == "gamepad_button" ||
         name == "gamepad_axis" || name == "touch_zone" ||
         name == "mouse_button";
}

bool IsKnownActionType(const std::string& name) {
  return name == "key" || name == "key_chord" || name == "mouse_click" ||
         name == "mouse_move" || name == "wheel" || name == "macro";
}

}  // namespace

std::string SourceTypeName(SourceType type) {
  switch (type) {
    case SourceType::key: return "key";
    case SourceType::gamepad_button: return "gamepad_button";
    case SourceType::gamepad_axis: return "gamepad_axis";
    case SourceType::touch_zone: return "touch_zone";
    case SourceType::mouse_button: return "mouse_button";
  }
  return "unknown";
}

SourceType ParseSourceType(const std::string& name) {
  if (name == "key") return SourceType::key;
  if (name == "gamepad_button") return SourceType::gamepad_button;
  if (name == "gamepad_axis") return SourceType::gamepad_axis;
  if (name == "touch_zone") return SourceType::touch_zone;
  if (name == "mouse_button") return SourceType::mouse_button;
  return SourceType::key;
}

std::string ActionTypeName(ActionType type) {
  switch (type) {
    case ActionType::key: return "key";
    case ActionType::key_chord: return "key_chord";
    case ActionType::mouse_click: return "mouse_click";
    case ActionType::mouse_move: return "mouse_move";
    case ActionType::wheel: return "wheel";
    case ActionType::macro: return "macro";
  }
  return "unknown";
}

ActionType ParseActionType(const std::string& name) {
  if (name == "key") return ActionType::key;
  if (name == "key_chord") return ActionType::key_chord;
  if (name == "mouse_click") return ActionType::mouse_click;
  if (name == "mouse_move") return ActionType::mouse_move;
  if (name == "wheel") return ActionType::wheel;
  if (name == "macro") return ActionType::macro;
  return ActionType::key;
}

bool InsideTouchZone(const Source& zone, float x, float y) {
  return x >= zone.x && x <= zone.x + zone.w && y >= zone.y &&
         y <= zone.y + zone.h;
}

bool KeyMapEngine::LoadProfile(const Profile& profile) {
  profile_ = profile;
  return true;
}

void KeyMapEngine::Clear() { profile_ = Profile{}; }

bool KeyMapEngine::Resolve(const Source& source, int value,
                           std::vector<Target>& targets) const {
  targets.clear();
  for (const auto& rule : profile_.rules) {
    if (!rule.enabled) continue;
    bool matched = false;
    switch (rule.source.type) {
      case SourceType::touch_zone:
        matched = source.type == SourceType::touch_zone &&
                  InsideTouchZone(rule.source, source.x, source.y);
        break;
      case SourceType::gamepad_axis:
        matched = source.type == SourceType::gamepad_axis &&
                  source.code == rule.source.code &&
                  ((rule.source.threshold >= 0 &&
                    value >= rule.source.threshold) ||
                   (rule.source.threshold < 0 &&
                    value <= rule.source.threshold));
        break;
      case SourceType::key:
      case SourceType::gamepad_button:
      case SourceType::mouse_button:
        matched = source.type == rule.source.type &&
                  source.code == rule.source.code;
        break;
    }
    if (matched) targets.push_back(rule.target);
  }
  return !targets.empty();
}

std::string KeyMapEngine::Serialize() const {
  std::ostringstream out;
  out << "profile " << (profile_.name.empty() ? "default" : profile_.name)
      << '\n';
  for (const auto& rule : profile_.rules) {
    out << "rule " << rule.id << ' ' << (rule.enabled ? 1 : 0) << ' '
        << SourceTypeName(rule.source.type) << ':' << rule.source.code;
    if (rule.source.type == SourceType::gamepad_axis) {
      out << ':' << rule.source.threshold;
    } else if (rule.source.type == SourceType::touch_zone) {
      out << ':' << rule.source.x << ',' << rule.source.y << ','
          << rule.source.w << ',' << rule.source.h;
    }
    out << ' ' << ActionTypeName(rule.target.type) << ':';
    switch (rule.target.type) {
      case ActionType::key:
      case ActionType::key_chord:
        for (std::size_t i = 0; i < rule.target.keys.size(); ++i) {
          if (i) out << ',';
          out << rule.target.keys[i];
        }
        break;
      case ActionType::mouse_click:
        out << rule.target.mouse_button;
        break;
      case ActionType::mouse_move:
        out << rule.target.dx << ':' << rule.target.dy;
        break;
      case ActionType::wheel:
        out << rule.target.dx << ':' << rule.target.dy;
        break;
      case ActionType::macro:
        for (std::size_t i = 0; i < rule.target.keys.size(); ++i) {
          if (i) out << ',';
          out << rule.target.keys[i];
        }
        out << ':' << rule.target.delay_ms;
        break;
    }
    out << '\n';
  }
  return out.str();
}

bool KeyMapEngine::Deserialize(const std::string& text) {
  Profile parsed;
  std::istringstream stream(text);
  std::string line;
  bool saw_profile = false;
  while (std::getline(stream, line)) {
    if (line.empty()) continue;
    std::istringstream fields(line);
    std::string kind;
    fields >> kind;
    if (kind == "profile") {
      if (saw_profile || !(fields >> parsed.name)) return false;
      saw_profile = true;
      continue;
    }
    if (kind != "rule" || !saw_profile) return false;

    Rule rule;
    int enabled = 0;
    std::string source_spec;
    std::string target_spec;
    if (!(fields >> rule.id >> enabled >> source_spec >> target_spec)) {
      return false;
    }
    rule.enabled = enabled != 0;

    const auto source_parts = Split(source_spec, ':');
    if (source_parts.size() < 2 || !IsKnownSourceType(source_parts[0])) {
      return false;
    }
    rule.source.type = ParseSourceType(source_parts[0]);
    if (!ParseUInt(source_parts[1], rule.source.code)) return false;
    if (rule.source.type == SourceType::gamepad_axis) {
      if (source_parts.size() < 3 ||
          !ParseInt(source_parts[2], rule.source.threshold)) {
        return false;
      }
    } else if (rule.source.type == SourceType::touch_zone) {
      if (source_parts.size() < 3) return false;
      const auto rect = Split(source_parts[2], ',');
      if (rect.size() != 4 || !ParseFloat(rect[0], rule.source.x) ||
          !ParseFloat(rect[1], rule.source.y) ||
          !ParseFloat(rect[2], rule.source.w) ||
          !ParseFloat(rect[3], rule.source.h)) {
        return false;
      }
    }

    const auto target_parts = Split(target_spec, ':');
    if (target_parts.size() < 2 || !IsKnownActionType(target_parts[0])) {
      return false;
    }
    rule.target.type = ParseActionType(target_parts[0]);
    switch (rule.target.type) {
      case ActionType::key:
      case ActionType::key_chord:
        if (!ParseKeys(target_parts[1], rule.target.keys)) return false;
        break;
      case ActionType::mouse_click:
        if (!ParseInt(target_parts[1], rule.target.mouse_button)) return false;
        break;
      case ActionType::mouse_move:
      case ActionType::wheel:
        if (target_parts.size() < 3 ||
            !ParseFloat(target_parts[1], rule.target.dx) ||
            !ParseFloat(target_parts[2], rule.target.dy)) {
          return false;
        }
        break;
      case ActionType::macro:
        if (target_parts.size() < 3 ||
            !ParseKeys(target_parts[1], rule.target.keys) ||
            !ParseUInt(target_parts[2], rule.target.delay_ms)) {
          return false;
        }
        break;
    }
    parsed.rules.push_back(std::move(rule));
  }
  if (!saw_profile) return false;
  profile_ = std::move(parsed);
  return true;
}

Profile DefaultFpsProfile() {
  Profile profile;
  profile.name = "FPS";

  Rule look;
  look.id = "look";
  look.source.type = SourceType::touch_zone;
  look.source.x = 0.5f;
  look.source.y = 0.0f;
  look.source.w = 0.5f;
  look.source.h = 0.75f;
  look.target.type = ActionType::mouse_move;
  look.target.dx = 1.0f;
  look.target.dy = 1.0f;
  profile.rules.push_back(look);

  Rule fire;
  fire.id = "fire";
  fire.source.type = SourceType::gamepad_button;
  fire.source.code = 0;
  fire.target.type = ActionType::mouse_click;
  fire.target.mouse_button = 0;
  profile.rules.push_back(fire);

  Rule reload;
  reload.id = "reload";
  reload.source.type = SourceType::gamepad_button;
  reload.source.code = 2;
  reload.target.type = ActionType::key;
  reload.target.keys = {0x52};  // R
  profile.rules.push_back(reload);

  Rule sprint;
  sprint.id = "sprint";
  sprint.source.type = SourceType::gamepad_axis;
  sprint.source.code = 1;
  sprint.source.threshold = 70;
  sprint.target.type = ActionType::key;
  sprint.target.keys = {0x10};  // Shift
  profile.rules.push_back(sprint);

  return profile;
}

Profile DefaultMobaProfile() {
  Profile profile;
  profile.name = "MOBA";

  Rule move;
  move.id = "move";
  move.source.type = SourceType::touch_zone;
  move.source.x = 0.0f;
  move.source.y = 0.45f;
  move.source.w = 0.45f;
  move.source.h = 0.55f;
  move.target.type = ActionType::mouse_move;
  move.target.dx = 1.0f;
  move.target.dy = 1.0f;
  profile.rules.push_back(move);

  Rule attack;
  attack.id = "attack";
  attack.source.type = SourceType::gamepad_button;
  attack.source.code = 0;
  attack.target.type = ActionType::mouse_click;
  attack.target.mouse_button = 0;
  profile.rules.push_back(attack);

  Rule spell;
  spell.id = "spell";
  spell.source.type = SourceType::gamepad_button;
  spell.source.code = 1;
  spell.target.type = ActionType::key;
  spell.target.keys = {0x51};  // Q
  profile.rules.push_back(spell);

  return profile;
}

}  // namespace keymap
}  // namespace crossdesk
