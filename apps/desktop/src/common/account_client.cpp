/*
 * CrossDesk account client (Windows WinHTTP).
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "common/account_client.h"

#include <nlohmann/json.hpp>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

namespace crossdesk {
namespace account {
namespace {

#ifdef _WIN32
std::wstring Widen(const std::string& text) {
  if (text.empty()) return {};
  const int size = MultiByteToWideChar(CP_UTF8, 0, text.data(),
                                       static_cast<int>(text.size()), nullptr, 0);
  std::wstring result(size, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                      &result[0], size);
  return result;
}

struct HttpResult {
  int status = 0;
  std::string body;
  std::string error;
};

HttpResult Request(const std::string& host, int port, const wchar_t* method,
                   const std::wstring& path, const std::string& body,
                   const std::string& token) {
  HttpResult result;
  HINTERNET session = WinHttpOpen(
      L"CrossDesk-Account/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
      WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (!session) {
    result.error = "无法初始化网络";
    return result;
  }
  WinHttpSetTimeouts(session, 5000, 5000, 10000, 15000);
  HINTERNET connect =
      WinHttpConnect(session, Widen(host).c_str(),
                     static_cast<INTERNET_PORT>(port), 0);
  if (!connect) {
    result.error = "无法连接账号服务";
    WinHttpCloseHandle(session);
    return result;
  }
  HINTERNET request = WinHttpOpenRequest(
      connect, method, path.c_str(), nullptr, WINHTTP_NO_REFERER,
      WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
  if (!request) {
    result.error = "无法创建请求";
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    return result;
  }
  std::wstring headers = L"Content-Type: application/json\r\n";
  if (!token.empty()) headers += L"Authorization: Bearer " + Widen(token) + L"\r\n";
  const BOOL sent = WinHttpSendRequest(
      request, headers.c_str(), static_cast<DWORD>(-1),
      body.empty() ? WINHTTP_NO_REQUEST_DATA
                   : const_cast<char*>(body.data()),
      static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0);
  if (!sent || !WinHttpReceiveResponse(request, nullptr)) {
    result.error = "账号服务请求失败";
  } else {
    DWORD status = 0;
    DWORD size = sizeof(status);
    WinHttpQueryHeaders(request,
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &size,
                        WINHTTP_NO_HEADER_INDEX);
    result.status = static_cast<int>(status);
    DWORD available = 0;
    do {
      available = 0;
      if (!WinHttpQueryDataAvailable(request, &available)) break;
      if (available == 0) break;
      std::string chunk(available, '\0');
      DWORD read = 0;
      if (!WinHttpReadData(request, &chunk[0], available, &read)) break;
      chunk.resize(read);
      result.body += chunk;
    } while (available > 0);
  }
  WinHttpCloseHandle(request);
  WinHttpCloseHandle(connect);
  WinHttpCloseHandle(session);
  return result;
}
#endif

Response Parse(const std::string& body, int status) {
  Response response;
  response.status = status;
  try {
    const auto json = nlohmann::json::parse(body);
    response.ok = json.value("ok", false);
    response.error = json.value("error", std::string());
    response.token = json.value("token", std::string());
    response.username = json.value("username", std::string());
    if (json.contains("devices") && json["devices"].is_array()) {
      for (const auto& item : json["devices"]) {
        Device device;
        device.id = item.value("device_id", std::string());
        device.alias = item.value("alias", std::string());
        device.password = item.value("password", std::string());
        device.online = item.value("online", false);
        if (!device.id.empty()) response.devices.push_back(std::move(device));
      }
    }
  } catch (...) {
    response.error = "账号服务返回格式错误";
  }
  if (!response.ok && response.error.empty()) response.error = "请求失败";
  return response;
}

}  // namespace

Response Login(const std::string& host, int port, const std::string& username,
               const std::string& password, bool create) {
#ifdef _WIN32
  nlohmann::json body = {{"username", username}, {"password", password}};
  const auto result = Request(host, port, L"POST",
                              create ? L"/api/account/register"
                                     : L"/api/account/login",
                              body.dump(), "");
  return Parse(result.body, result.status);
#else
  (void)host; (void)port; (void)username; (void)password; (void)create;
  return Response{false, 0, "当前平台暂不支持账号登录"};
#endif
}

Response Devices(const std::string& host, int port, const std::string& token) {
#ifdef _WIN32
  const auto result = Request(host, port, L"GET", L"/api/account/devices", "",
                              token);
  return Parse(result.body, result.status);
#else
  (void)host; (void)port; (void)token;
  return Response{false, 0, "当前平台暂不支持账号登录"};
#endif
}

}  // namespace account
}  // namespace crossdesk
