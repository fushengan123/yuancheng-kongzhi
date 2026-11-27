/*
 * CrossDesk feature/parity - Wake-on-LAN tests.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "wake_on_lan.h"

#include <iostream>

namespace {

bool Check(const char* name, bool value) {
  if (!value) std::cerr << "FAIL: " << name << '\n';
  return value;
}

}  // namespace

int main() {
  bool ok = true;
  using namespace crossdesk::wol;

  uint8_t mac[6]{};
  ok &= Check("parse colon mac", ParseMac("AA:BB:CC:DD:EE:FF", mac));
  ok &= Check("format mac", FormatMac(mac) == "AA:BB:CC:DD:EE:FF");
  ok &= Check("parse dash/lower mac", ParseMac("aa-bb-cc-dd-ee-ff", mac));
  ok &= Check("reject short mac", !ParseMac("AA:BB:CC", mac));
  ok &= Check("reject bad mac", !ParseMac("AA:BB:CC:DD:EE:ZZ", mac));

  const auto packet = BuildMagicPacket(mac);
  ok &= Check("packet size", packet.size() == 102);
  bool preamble = true;
  for (int i = 0; i < 6; ++i) preamble &= packet[i] == 0xFF;
  ok &= Check("packet preamble", preamble);
  bool repeated = true;
  for (int i = 0; i < 16; ++i) {
    for (int j = 0; j < 6; ++j) {
      repeated &= packet[6 + i * 6 + j] == mac[j];
    }
  }
  ok &= Check("packet repeats mac", repeated);

  std::string error;
  ok &= Check("reject invalid broadcast",
              !SendWakeOnLan("not-an-ip", 9, "AA:BB:CC:DD:EE:FF", &error));

  std::cout << (ok ? "wake_on_lan_test: PASS\n" : "wake_on_lan_test: FAIL\n");
  return ok ? 0 : 1;
}
