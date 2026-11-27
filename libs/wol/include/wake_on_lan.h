/*
 * CrossDesk feature/parity - Wake-on-LAN helper.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef CROSSDESK_WAKE_ON_LAN_H_
#define CROSSDESK_WAKE_ON_LAN_H_

#include <cstdint>
#include <string>
#include <vector>

namespace crossdesk {
namespace wol {

bool ParseMac(const std::string& text, uint8_t mac[6]);
std::vector<uint8_t> BuildMagicPacket(const uint8_t mac[6]);
std::string FormatMac(const uint8_t mac[6]);

// Sends one magic packet to a IPv4 broadcast address (for example
// 192.168.1.255) on the given UDP port. Returns false and fills error on
// socket failure. Hosts normally use port 9 or 7.
bool SendWakeOnLan(const std::string& broadcast_address, uint16_t port,
                   const std::string& mac, std::string* error);

}  // namespace wol
}  // namespace crossdesk

#endif  // CROSSDESK_WAKE_ON_LAN_H_
