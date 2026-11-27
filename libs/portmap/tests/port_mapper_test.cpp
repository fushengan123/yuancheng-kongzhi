/*
 * CrossDesk feature/parity - LAN port mapping test.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <atomic>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include "port_mapper.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using socket_t = SOCKET;
constexpr socket_t kInvalidSocket = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using socket_t = int;
constexpr socket_t kInvalidSocket = -1;
#endif

namespace {

void CloseSocket(socket_t socket) {
#ifdef _WIN32
  closesocket(socket);
#else
  close(socket);
#endif
}

bool Check(const char* name, bool value) {
  std::cout << (value ? "PASS " : "FAIL ") << name << std::endl;
  return value;
}

class EchoServer {
 public:
  bool Start() {
#ifdef _WIN32
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return false;
#endif
    listener_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener_ == kInvalidSocket) return false;
    int reuse = 1;
    setsockopt(listener_, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&reuse), sizeof(reuse));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;
    if (bind(listener_, reinterpret_cast<sockaddr*>(&address),
             sizeof(address)) != 0 ||
        listen(listener_, 4) != 0) {
      return false;
    }
    sockaddr_in bound{};
#ifdef _WIN32
    int size = sizeof(bound);
#else
    socklen_t size = sizeof(bound);
#endif
    if (getsockname(listener_, reinterpret_cast<sockaddr*>(&bound), &size) !=
        0) {
      return false;
    }
    port_ = ntohs(bound.sin_port);
    thread_ = std::thread([this] { Loop(); });
    return true;
  }

  uint16_t port() const { return port_; }

  void Stop() {
    running_.store(false);
    if (listener_ != kInvalidSocket) {
#ifdef _WIN32
      shutdown(listener_, SD_BOTH);
#else
      shutdown(listener_, SHUT_RDWR);
#endif
      CloseSocket(listener_);
      listener_ = kInvalidSocket;
    }
    if (thread_.joinable()) thread_.join();
  }

 private:
  void Loop() {
    while (running_.load()) {
      socket_t client = accept(listener_, nullptr, nullptr);
      if (client == kInvalidSocket) break;
      char buffer[256];
      const int received = recv(client, buffer, sizeof(buffer), 0);
      if (received > 0) send(client, buffer, received, 0);
      CloseSocket(client);
    }
  }

  socket_t listener_ = kInvalidSocket;
  uint16_t port_ = 0;
  std::atomic<bool> running_{true};
  std::thread thread_;
};

bool ConnectAndEcho(uint16_t port, const std::string& payload) {
  socket_t client = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (client == kInvalidSocket) return false;
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
  if (connect(client, reinterpret_cast<sockaddr*>(&address),
              sizeof(address)) != 0) {
    CloseSocket(client);
    return false;
  }
  if (send(client, payload.data(), static_cast<int>(payload.size()), 0) !=
      static_cast<int>(payload.size())) {
    CloseSocket(client);
    return false;
  }
  std::string echoed(payload.size(), '\0');
  int total = 0;
  while (total < static_cast<int>(echoed.size())) {
    const int received =
        recv(client, &echoed[total], static_cast<int>(echoed.size()) - total, 0);
    if (received <= 0) break;
    total += received;
  }
  CloseSocket(client);
  return total == static_cast<int>(payload.size()) && echoed == payload;
}

}  // namespace

int main() {
  bool ok = true;
  EchoServer echo;
  ok &= Check("echo server starts", echo.Start());

  crossdesk::portmap::MappingRule rule{};
  rule.session_id = 7;
  rule.listen_port = 45231;
  rule.target_host = "127.0.0.1";
  rule.target_port = echo.port();
  ok &= Check("port mapper starts",
              crossdesk::portmap::PortMapper::Instance().Start(rule));
  ok &= Check("active count is 1",
              crossdesk::portmap::PortMapper::Instance().ActiveCount() == 1);
  ok &= Check("duplicate session rejected",
              !crossdesk::portmap::PortMapper::Instance().Start(rule));

  bool echoed = false;
  for (int attempt = 0; attempt < 50 && !echoed; ++attempt) {
    echoed = ConnectAndEcho(rule.listen_port, "crossdesk-port-map");
    if (!echoed) {
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
  }
  ok &= Check("mapped connection echoes payload", echoed);

  crossdesk::portmap::PortMapper::Instance().Stop(rule.session_id);
  ok &= Check("active count is 0 after stop",
              crossdesk::portmap::PortMapper::Instance().ActiveCount() == 0);
  ok &= Check("stop is idempotent", [&] {
    crossdesk::portmap::PortMapper::Instance().Stop(rule.session_id);
    return true;
  }());

  echo.Stop();
  return ok ? 0 : 1;
}
