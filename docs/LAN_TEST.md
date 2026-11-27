# 局域网直连测试（Windows 被控端 + iOS 自签控制端）

本测试用于在**不依赖云端信令**的情况下，验证 `feature/parity` 分支上的按键映射、手柄协议、WOL、远程终端等改动。
iOS 端在仓库 GitHub Actions 中编译，本机不需要 Xcode。

## 1. 测试拓扑

- 被控端：Windows 10/11，运行本分支构建的便携版 CrossDesk。
- 控制端：iPhone/iPad（iOS 16+），安装仓库编译的未签名 IPA，用个人 Apple ID 通过 Sideloadly / AltStore 自签安装。
- 网络：两端在同一局域网或 VPN，IPv4 互相可达；默认 IP 直连端口为 **52888**（TCP + UDP）。

## 2. 构建产物

在镜像仓库 `fushengan123/yuancheng-kongzhi` 的 `feature/parity` 分支触发 **LAN Test Builds** 工作流：

- `crossdesk-lan-win-x64.zip`：Windows 便携测试版。
- `crossdesk-lan-ios-unsigned.ipa`：未签名 IPA，供自签工具安装。
- `crossdesk-lan-ios-unsigned.app.zip`：未签名 `.app`，备用。

iOS 默认 Bundle ID 为 `com.lantest.crossdesk`，避免使用原作者标识。

## 3. Windows 被控端准备

1. 解压 `crossdesk-lan-win-x64.zip`，双击 `CrossDesk.exe` 运行。
2. 进入 **设置 → 常规 → 网络连接**，确认 **启用 IP 直连** 已开启并保存（默认开启）。
3. 关闭 Windows 防火墙拦截，或用管理员 PowerShell 运行：

   ```powershell
   powershell -ExecutionPolicy Bypass -File scripts\lan_preflight.ps1 -Port 52888 -AddFirewallRule
   ```

   如果只需检查不修改防火墙：

   ```powershell
   powershell -ExecutionPolicy Bypass -File scripts\lan_preflight.ps1 -Port 52888
   ```

4. 在主窗口左上角切换到 **IP 直连模式**，记下显示的 `IP:端口` 和 **6 位 IP 直连密码**（区分大小写）。

## 4. iOS 自签安装

1. 在 Windows/macOS 上安装 Sideloadly 或 AltStore，并登录个人 Apple ID。
2. 导入 `crossdesk-lan-ios-unsigned.ipa`，保持或改成唯一 Bundle ID（例如 `com.lantest.crossdesk`），开始自签安装。
3. 首次启动前在 iOS **设置 → 通用 → VPN 与设备管理** 中信任对应的开发者证书。
4. 首次打开 App 时，系统弹出“允许访问本地网络”请选择 **允许**。
5. 在首页 **设备 ID 或 IP 地址** 输入 Windows 端显示的 `IP:端口`，点击 **连接**，输入 6 位密码。

## 5. 功能测试矩阵

| 编号 | 测试项 | 操作 | 通过标准 |
|---|---|---|---|
| T1 | 局域网建连 | iOS 输入 `IP:52888` + 密码 | 画面出现，状态显示 P2P 直连，无云端信令 |
| T2 | 视频/音频 | 观察画面，打开远程音频 | 画面流畅，可听到远端声音 |
| T3 | 基础键鼠 | 虚拟键盘输入文本、鼠标点击/移动 | 远端正确响应 |
| T4 | 按键映射 | 在 iOS 悬浮菜单进入“按键映射”，添加一条如 `key:65 → key_chord:ctrl,key:86` 的规则并连接测试 | 远端收到映射后的组合键，而非原始按键 |
| T5 | 手柄协议 | 若有手柄/蓝牙手柄，发送按钮或轴事件 | 远端收到 `gamepad_state`，按键映射可把手柄事件转成键鼠动作 |
| T6 | 远程终端 | 连接后发送 `terminal_open/data/close` 控制消息 | 远端进程启动、输出回传、关闭后进程结束 |
| T7 | WOL 模块 | 在同一 LAN 内用已验证的 MAC 调用 `wake_on_lan` | 目标网卡收到 magic packet（模块单测已通过） |
| T8 | 文件/剪贴板 | 双向传文件，远端复制文本后本地粘贴 | 文件完整，剪贴板同步 |
| T9 | 隐私屏 | 打开隐私屏 | 远端显示器黑屏但会话不中断 |
| T10 | 断线重连 | 关闭 Wi-Fi 再恢复 | 可重新输入地址与密码连接 |

## 6. 已知边界

- 本版本只是**功能验证构建**，没有做品牌 Logo 和文案替换。
- 手柄硬件采集与震动反馈尚未接入各端 UI；协议与注入链路已就绪。
- WOL 目前是库与单测，未做图形入口。
- 端口映射、高帧率/HDR、账号云同步、手机副屏按用户要求暂缓。
- 衍生版本必须继续遵守 GPL-3.0-only：保留 `LICENSE`、作者声明和第三方许可证清单。
