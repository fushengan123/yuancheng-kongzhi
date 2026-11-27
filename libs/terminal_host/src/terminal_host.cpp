/*
 * CrossDesk feature/parity - remote terminal host.
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "terminal_host.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <csignal>
#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace crossdesk {
namespace terminal {

#ifdef _WIN32
struct TerminalHost::Session {
  HANDLE process = nullptr;
  HANDLE stdin_write = nullptr;
  HANDLE stdout_read = nullptr;
};
#else
struct TerminalHost::Session {
  pid_t pid = -1;
};
#endif

TerminalHost& TerminalHost::Instance() {
  static TerminalHost instance;
  return instance;
}

bool TerminalHost::Open(uint32_t session_id, uint32_t cols, uint32_t rows,
                        const std::string& shell) {
  (void)cols;
  (void)rows;
  std::lock_guard<std::mutex> lock(mutex_);
  if (sessions_.find(session_id) != sessions_.end()) return false;
#ifdef _WIN32
  SECURITY_ATTRIBUTES sa{};
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;
  HANDLE stdin_read = nullptr;
  HANDLE stdin_write = nullptr;
  HANDLE stdout_read = nullptr;
  HANDLE stdout_write = nullptr;
  if (!CreatePipe(&stdin_read, &stdin_write, &sa, 0) ||
      !CreatePipe(&stdout_read, &stdout_write, &sa, 0)) {
    if (stdin_read) CloseHandle(stdin_read);
    if (stdin_write) CloseHandle(stdin_write);
    if (stdout_read) CloseHandle(stdout_read);
    if (stdout_write) CloseHandle(stdout_write);
    return false;
  }
  SetHandleInformation(stdin_write, HANDLE_FLAG_INHERIT, 0);
  SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOA si{};
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE;
  si.hStdInput = stdin_read;
  si.hStdOutput = stdout_write;
  si.hStdError = stdout_write;
  PROCESS_INFORMATION pi{};
  std::string command = shell.empty() ? std::string("cmd.exe") : shell;
  BOOL created = CreateProcessA(nullptr, &command[0], nullptr, nullptr, TRUE,
                                CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
  CloseHandle(stdin_read);
  CloseHandle(stdout_write);
  if (!created) {
    CloseHandle(stdin_write);
    CloseHandle(stdout_read);
    return false;
  }
  CloseHandle(pi.hThread);

  auto session = std::make_unique<Session>();
  session->process = pi.hProcess;
  session->stdin_write = stdin_write;
  session->stdout_read = stdout_read;
  sessions_.emplace(session_id, std::move(session));
  return true;
#else
  int pipes[2];
  if (pipe(pipes) != 0) return false;
  pid_t pid = fork();
  if (pid == 0) {
    dup2(pipes[1], STDOUT_FILENO);
    dup2(pipes[1], STDERR_FILENO);
    close(pipes[0]);
    close(pipes[1]);
    execl("/bin/sh", "sh", nullptr);
    _exit(127);
  }
  close(pipes[1]);
  if (pid < 0) {
    close(pipes[0]);
    return false;
  }
  auto session = std::make_unique<Session>();
  session->pid = pid;
  sessions_.emplace(session_id, std::move(session));
  return true;
#endif
}

bool TerminalHost::Write(uint32_t session_id, const std::string& data) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = sessions_.find(session_id);
  if (it == sessions_.end()) return false;
#ifdef _WIN32
  DWORD written = 0;
  return WriteFile(it->second->stdin_write, data.data(),
                   static_cast<DWORD>(data.size()), &written, nullptr) != FALSE &&
         written == data.size();
#else
  (void)data;
  return it->second->pid > 0;
#endif
}

std::string TerminalHost::Read(uint32_t session_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = sessions_.find(session_id);
  if (it == sessions_.end()) return {};
#ifdef _WIN32
  DWORD available = 0;
  if (!PeekNamedPipe(it->second->stdout_read, nullptr, 0, nullptr, &available,
                     nullptr) ||
      available == 0) {
    return {};
  }
  std::string buffer(available, '\0');
  DWORD read = 0;
  if (!ReadFile(it->second->stdout_read, &buffer[0], available, &read,
                nullptr)) {
    return {};
  }
  buffer.resize(read);
  return buffer;
#else
  return {};
#endif
}

void TerminalHost::Close(uint32_t session_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = sessions_.find(session_id);
  if (it == sessions_.end()) return;
#ifdef _WIN32
  TerminateProcess(it->second->process, 0);
  CloseHandle(it->second->process);
  CloseHandle(it->second->stdin_write);
  CloseHandle(it->second->stdout_read);
#else
  kill(it->second->pid, SIGTERM);
  waitpid(it->second->pid, nullptr, 0);
#endif
  sessions_.erase(it);
}

bool TerminalHost::Has(uint32_t session_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  return sessions_.find(session_id) != sessions_.end();
}

}  // namespace terminal
}  // namespace crossdesk
