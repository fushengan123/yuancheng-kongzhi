/*
 * CrossDesk account client (Windows WinHTTP).
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef CROSSDESK_ACCOUNT_CLIENT_H_
#define CROSSDESK_ACCOUNT_CLIENT_H_

#include <string>
#include <vector>

namespace crossdesk {
namespace account {

inline constexpr const char* kDefaultHost =
    "account.161.153.102.103.sslip.io";
inline constexpr int kDefaultPort = 443;

struct Device {
  std::string id;
  std::string alias;
  std::string password;
  bool online = false;
};

struct Response {
  bool ok = false;
  int status = 0;
  std::string error;
  std::string token;
  std::string username;
  std::vector<Device> devices;
};

Response Login(const std::string& host, int port, const std::string& username,
               const std::string& password, bool create);
Response Devices(const std::string& host, int port, const std::string& token);

}  // namespace account
}  // namespace crossdesk

#endif  // CROSSDESK_ACCOUNT_CLIENT_H_
