/*
 * CrossDesk feature/parity - Wake-on-LAN helper.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "wake_on_lan.h"

#include <algorithm>
#include <cctype>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace crossdesk {
namespace wol {
namespace {

int HexValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

}  // namespace

bool ParseMac(const std::string& text, uint8_t mac[6]) {
  uint8_t parsed[6]{};
  std::size_t index = 0;
  std::size_t cursor = 0;
  while (cursor < text.size() && index < 6) {
    while (cursor < text.size() &&
           (std::isspace(static_cast<unsigned char>(text[cursor])) ||
            text[cursor] == ':' || text[cursor] == '-')) {
      ++cursor;
    }
    if (cursor + 1 >= text.size()) return false;
    const int high = HexValue(text[cursor]);
    const int low = HexValue(text[cursor + 1]);
    if (high < 0 || low < 0) return false;
    parsed[index++] = static_cast<uint8_t>((high << 4) | low);
    cursor += 2;
  }
  while (cursor < text.size() &&
         (std::isspace(static_cast<unsigned char>(text[cursor])) ||
          text[cursor] == ':' || text[cursor] == '-')) {
    ++cursor;
  }
  if (index != 6 || cursor != text.size()) return false;
  std::memcpy(mac, parsed, sizeof(parsed));
  return true;
}

std::vector<uint8_t> BuildMagicPacket(const uint8_t mac[6]) {
  std::vector<uint8_t> packet;
  packet.reserve(102);
  packet.insert(packet.end(), 6, 0xFF);
  for (int i = 0; i < 16; ++i) {
    packet.insert(packet.end(), mac, mac + 6);
  }
  return packet;
}

std::string FormatMac(const uint8_t mac[6]) {
  static const char* digits = "0123456789ABCDEF";
  std::string result;
  for (int i = 0; i < 6; ++i) {
    if (i) result.push_back(':');
    result.push_back(digits[(mac[i] >> 4) & 0x0F]);
    result.push_back(digits[mac[i] & 0x0F]);
  }
  return result;
}

bool SendWakeOnLan(const std::string& broadcast_address, uint16_t port,
                   const std::string& mac, std::string* error) {
  uint8_t parsed[6]{};
  if (!ParseMac(mac, parsed)) {
    if (error) *error = "invalid_mac";
    return false;
  }
#ifdef _WIN32
  WSADATA wsa{};
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    if (error) *error = "wsa_startup_failed";
    return false;
  }
#endif
  int fd = static_cast<int>(::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
  if (fd < 0) {
    if (error) *error = "socket_failed";
#ifdef _WIN32
    WSACleanup();
#endif
    return false;
  }
  int broadcast = 1;
  ::setsockopt(fd, SOL_SOCKET, SO_BROADCAST,
               reinterpret_cast<const char*>(&broadcast), sizeof(broadcast));

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  if (::inet_pton(AF_INET, broadcast_address.c_str(), &address.sin_addr) != 1) {
    if (error) *error = "invalid_broadcast_address";
#ifdef _WIN32
    ::closesocket(fd);
    WSACleanup();
#else
    ::close(fd);
#endif
    return false;
  }
  const auto packet = BuildMagicPacket(parsed);
  const bool sent =
      ::sendto(fd, reinterpret_cast<const char*>(packet.data()),
               static_cast<int>(packet.size()), 0,
               reinterpret_cast<sockaddr*>(&address), sizeof(address)) ==
      static_cast<int>(packet.size());
  if (!sent && error) *error = "send_failed";
#ifdef _WIN32
  ::closesocket(fd);
  WSACleanup();
#else
  ::close(fd);
#endif
  return sent;
}

}  // namespace wol
}  // namespace crossdesk
