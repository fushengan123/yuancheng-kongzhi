/*
 * CrossDesk feature/parity - LAN port mapping.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef CROSSDESK_PORT_MAPPER_H_
#define CROSSDESK_PORT_MAPPER_H_

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace crossdesk {
namespace portmap {

struct MappingRule {
  uint32_t session_id = 0;
  uint16_t listen_port = 0;
  std::string target_host = "127.0.0.1";
  uint16_t target_port = 0;
};

// Host-side TCP forwarder. A rule opens a listening socket on the host and
// relays each accepted connection to target_host:target_port. The client then
// reaches the mapped service through the host's LAN address.
class PortMapper {
 public:
  static PortMapper& Instance();

  bool Start(const MappingRule& rule);
  void Stop(uint32_t session_id);
  void StopAll();
  bool Has(uint32_t session_id);
  uint32_t ActiveCount();

  PortMapper(const PortMapper&) = delete;
  PortMapper& operator=(const PortMapper&) = delete;

 private:
  PortMapper() = default;
  struct Session;
  std::mutex mutex_;
  std::map<uint32_t, std::shared_ptr<Session>> sessions_;
};

}  // namespace portmap
}  // namespace crossdesk

#endif  // CROSSDESK_PORT_MAPPER_H_
