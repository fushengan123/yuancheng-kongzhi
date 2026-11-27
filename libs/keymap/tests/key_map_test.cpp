/*
 * CrossDesk feature/parity - key mapping engine tests.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "key_map.h"

#include <iostream>
#include <string>

namespace {

bool Check(const char* name, bool value) {
  if (!value) {
    std::cerr << "FAIL: " << name << '\n';
  }
  return value;
}

}  // namespace

int main() {
  bool ok = true;
  using namespace crossdesk::keymap;

  Profile fps = DefaultFpsProfile();
  ok &= Check("fps profile has rules", fps.rules.size() >= 4);
  ok &= Check("fps profile name", fps.name == "FPS");

  KeyMapEngine engine;
  ok &= Check("load profile", engine.LoadProfile(fps));

  Source fire{};
  fire.type = SourceType::gamepad_button;
  fire.code = 0;
  std::vector<Target> targets;
  ok &= Check("resolve gamepad fire", engine.Resolve(fire, 0, targets));
  ok &= Check("fire target is click",
              targets.size() == 1 &&
                  targets[0].type == ActionType::mouse_click &&
                  targets[0].mouse_button == 0);

  Source reload{};
  reload.type = SourceType::gamepad_button;
  reload.code = 2;
  ok &= Check("resolve reload", engine.Resolve(reload, 0, targets));
  ok &= Check("reload key is R",
              targets.size() == 1 && targets[0].keys.size() == 1 &&
                  targets[0].keys[0] == 0x52);

  Source axis{};
  axis.type = SourceType::gamepad_axis;
  axis.code = 1;
  ok &= Check("axis over threshold", engine.Resolve(axis, 90, targets));
  ok &= Check("axis below threshold", !engine.Resolve(axis, 20, targets));

  Source touch{};
  touch.type = SourceType::touch_zone;
  touch.x = 0.8f;
  touch.y = 0.3f;
  ok &= Check("touch inside look zone", engine.Resolve(touch, 0, targets));
  ok &= Check("look target is mouse move",
              targets.size() == 1 &&
                  targets[0].type == ActionType::mouse_move);

  touch.x = 0.2f;
  ok &= Check("touch outside look zone", !engine.Resolve(touch, 0, targets));

  const std::string serialized = engine.Serialize();
  KeyMapEngine restored;
  ok &= Check("deserialize", restored.Deserialize(serialized));
  ok &= Check("roundtrip name", restored.profile().name == "FPS");
  ok &= Check("roundtrip rules",
              restored.profile().rules.size() == fps.rules.size());
  ok &= Check("roundtrip resolve", restored.Resolve(fire, 0, targets));

  ok &= Check("reject bad profile", !restored.Deserialize("garbage"));
  ok &= Check("reject bad rule",
              !restored.Deserialize(
                  "profile X\nrule broken 1 key:1 bogus:2\n"));

  KeyMapEngine moba;
  ok &= Check("load moba", moba.LoadProfile(DefaultMobaProfile()));
  ok &= Check("moba spell key", [] {
    KeyMapEngine e;
    e.LoadProfile(DefaultMobaProfile());
    Source s{};
    s.type = SourceType::gamepad_button;
    s.code = 1;
    std::vector<Target> out;
    if (!e.Resolve(s, 0, out) || out.size() != 1 || out[0].keys.size() != 1 ||
        out[0].keys[0] != 0x51) {
      return false;
    }
    return true;
  }());

  std::cout << (ok ? "key_map_test: PASS\n" : "key_map_test: FAIL\n");
  return ok ? 0 : 1;
}
