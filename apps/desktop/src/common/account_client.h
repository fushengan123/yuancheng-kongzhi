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
// Bind this device to the signed-in account so it appears in the account's
// device list. device_password is the local access code (1-6 alphanumeric).
Response Bind(const std::string& host, int port, const std::string& token,
              const std::string& device_id, const std::string& alias,
              const std::string& device_password);

}  // namespace account
}  // namespace crossdesk

#endif  // CROSSDESK_ACCOUNT_CLIENT_H_
