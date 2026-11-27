/*
 * CrossDesk feature/parity - LAN port mapping.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "port_mapper.h"

#include <atomic>
#include <thread>
#include <vector>

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

namespace crossdesk {
namespace portmap {
namespace {

void CloseSocket(socket_t socket) {
#ifdef _WIN32
  closesocket(socket);
#else
  close(socket);
#endif
}

void Pump(socket_t from, socket_t to, std::atomic<bool>* running) {
  std::vector<char> buffer(16 * 1024);
  while (running->load()) {
    const int received = recv(from, buffer.data(),
                              static_cast<int>(buffer.size()), 0);
    if (received <= 0) break;
    int sent = 0;
    while (sent < received && running->load()) {
      const int chunk = send(to, buffer.data() + sent, received - sent, 0);
      if (chunk <= 0) {
        running->store(false);
        return;
      }
      sent += chunk;
    }
  }
  running->store(false);
#ifdef _WIN32
  shutdown(from, SD_BOTH);
  shutdown(to, SD_BOTH);
#else
  shutdown(from, SHUT_RDWR);
  shutdown(to, SHUT_RDWR);
#endif
}

bool ConnectTo(const std::string& host, uint16_t port, socket_t* out) {
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo* result = nullptr;
  const std::string service = std::to_string(port);
  if (getaddrinfo(host.c_str(), service.c_str(), &hints, &result) != 0) {
    return false;
  }
  socket_t socket = kInvalidSocket;
  for (addrinfo* entry = result; entry != nullptr; entry = entry->ai_next) {
    socket = ::socket(entry->ai_family, entry->ai_socktype, entry->ai_protocol);
    if (socket == kInvalidSocket) continue;
    if (connect(socket, entry->ai_addr,
                static_cast<int>(entry->ai_addrlen)) == 0) {
      break;
    }
    CloseSocket(socket);
    socket = kInvalidSocket;
  }
  freeaddrinfo(result);
  if (socket == kInvalidSocket) return false;
  *out = socket;
  return true;
}

}  // namespace

struct PortMapper::Session {
  uint32_t session_id = 0;
  uint16_t listen_port = 0;
  std::string target_host;
  uint16_t target_port = 0;
  socket_t listener = kInvalidSocket;
  std::atomic<bool> running{true};
  std::thread accept_thread;

  ~Session() {
    running.store(false);
    if (listener != kInvalidSocket) {
#ifdef _WIN32
      shutdown(listener, SD_BOTH);
#else
      shutdown(listener, SHUT_RDWR);
#endif
      CloseSocket(listener);
      listener = kInvalidSocket;
    }
    if (accept_thread.joinable()) accept_thread.join();
  }
};

PortMapper& PortMapper::Instance() {
  static PortMapper instance;
  return instance;
}

bool PortMapper::Start(const MappingRule& rule) {
  if (rule.session_id == 0 || rule.listen_port == 0 ||
      rule.target_port == 0 || rule.target_host.empty()) {
    return false;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (sessions_.find(rule.session_id) != sessions_.end()) return false;

#ifdef _WIN32
  static bool winsock_ready = [] {
    WSADATA data{};
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
  }();
  if (!winsock_ready) return false;
#endif

  socket_t listener = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listener == kInvalidSocket) return false;
  int reuse = 1;
  setsockopt(listener, SOL_SOCKET, SO_REUSEADDR,
             reinterpret_cast<const char*>(&reuse), sizeof(reuse));

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(rule.listen_port);
  if (bind(listener, reinterpret_cast<sockaddr*>(&address),
           sizeof(address)) != 0 ||
      listen(listener, 8) != 0) {
    CloseSocket(listener);
    return false;
  }

  auto session = std::make_shared<Session>();
  session->session_id = rule.session_id;
  session->listen_port = rule.listen_port;
  session->target_host = rule.target_host;
  session->target_port = rule.target_port;
  session->listener = listener;
  Session* raw = session.get();
  session->accept_thread = std::thread([raw] {
    while (raw->running.load()) {
      sockaddr_in peer{};
#ifdef _WIN32
      int peer_size = sizeof(peer);
#else
      socklen_t peer_size = sizeof(peer);
#endif
      socket_t client = accept(raw->listener,
                               reinterpret_cast<sockaddr*>(&peer), &peer_size);
      if (client == kInvalidSocket) break;
      socket_t target = kInvalidSocket;
      if (!ConnectTo(raw->target_host, raw->target_port, &target)) {
        CloseSocket(client);
        continue;
      }
      auto relay = std::make_shared<std::atomic<bool>>(true);
      std::thread up([client, target, relay] {
        Pump(client, target, relay.get());
      });
      std::thread down([target, client, relay] {
        Pump(target, client, relay.get());
      });
      up.detach();
      down.detach();
    }
  });

  sessions_.emplace(rule.session_id, std::move(session));
  return true;
}

void PortMapper::Stop(uint32_t session_id) {
  std::shared_ptr<Session> session;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = sessions_.find(session_id);
    if (found == sessions_.end()) return;
    session = found->second;
    sessions_.erase(found);
  }
  session.reset();
}

void PortMapper::StopAll() {
  std::map<uint32_t, std::shared_ptr<Session>> sessions;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions.swap(sessions_);
  }
  sessions.clear();
}

bool PortMapper::Has(uint32_t session_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  return sessions_.find(session_id) != sessions_.end();
}

uint32_t PortMapper::ActiveCount() {
  std::lock_guard<std::mutex> lock(mutex_);
  return static_cast<uint32_t>(sessions_.size());
}

}  // namespace portmap
}  // namespace crossdesk
