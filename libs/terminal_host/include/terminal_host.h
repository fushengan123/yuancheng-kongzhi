/*
 * CrossDesk feature/parity - remote terminal host.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef CROSSDESK_TERMINAL_HOST_H_
#define CROSSDESK_TERMINAL_HOST_H_

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace crossdesk {
namespace terminal {

class TerminalHost {
 public:
  static TerminalHost& Instance();

  bool Open(uint32_t session_id, uint32_t cols, uint32_t rows,
            const std::string& shell);
  bool Write(uint32_t session_id, const std::string& data);
  std::string Read(uint32_t session_id);
  void Close(uint32_t session_id);
  bool Has(uint32_t session_id);

  TerminalHost(const TerminalHost&) = delete;
  TerminalHost& operator=(const TerminalHost&) = delete;

 private:
  TerminalHost() = default;
  struct Session;
  std::mutex mutex_;
  std::map<uint32_t, std::unique_ptr<Session>> sessions_;
};

}  // namespace terminal
}  // namespace crossdesk

#endif  // CROSSDESK_TERMINAL_HOST_H_
