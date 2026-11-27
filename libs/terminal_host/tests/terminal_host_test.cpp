/*
 * CrossDesk feature/parity - remote terminal host tests.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "terminal_host.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

namespace {

bool Check(const char* name, bool value) {
  if (!value) std::cerr << "FAIL: " << name << '\n';
  return value;
}

}  // namespace

int main() {
  bool ok = true;
  auto& host = crossdesk::terminal::TerminalHost::Instance();
  ok &= Check("no session initially", !host.Has(1));
  ok &= Check("duplicate open rejected",
              host.Open(1, 80, 24, "cmd.exe /c echo ready") &&
                  !host.Open(1, 80, 24, "cmd.exe /c echo ready"));

  std::string output;
  for (int i = 0; i < 200 && output.find("ready") == std::string::npos; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    output += host.Read(1);
  }
  ok &= Check("command output captured", output.find("ready") != std::string::npos);

  host.Close(1);
  ok &= Check("session closed", !host.Has(1));

  std::cout << (ok ? "terminal_host_test: PASS\n" : "terminal_host_test: FAIL\n");
  return ok ? 0 : 1;
}
