# UU 远程功能还原清单（基于本机安装包）

本文件记录从本机安装的 UU 远程 Windows 客户端提取到的功能面，用于对照 CrossDesk 的补齐范围。
只记录可观察到的协议/模块/命令名称，不复制其代码或资源。

## 证据来源

| 目标 | 路径 | 说明 |
| --- | --- | --- |
| 主程序 | `D:\GameViewer\bin\GameViewer.exe` | Qt5 + WebView2 客户端 |
| 被控服务 | `D:\GameViewer\bin\GameViewerServer.exe` | 被控端采集/输入/设备能力 |
| 流媒体 | `D:\GameViewer\bin\streamer.dll` | 编码、HDR、4:4:4、144fps 相关字符串 |
| 命令行 | `D:\GameViewer\bin\uuyc-cli.exe` | 设备/终端/协助/云电脑命令 |
| 终端多路复用 | `D:\GameViewer\bin\uuyc-mux.exe` | 类 tmux 会话（psmux） |
| 终端桥 | `D:\GameViewer\bin\conpty_bridge.exe` | ConPTY/Shell 桥接 |
| 版本 | 注册表 `GameViewer` | `UU远程 4.47.1.5286` |

本机只有 Windows 客户端；Android/iOS 未安装。提取到的 `.gv_pb.*` 是客户端/服务端共用协议，功能面可用于三端。

## 从 uuyc-cli 提取到的命令面

```text
uuyc-cli
  status                 应用诊断
  user                   账号命令
  device ls|info|connect|disconnect|status
  cloudpc                云电脑管理
  assist id|reset-code|set-code|allow|start|status
  term -d DEVICE new|attach|ls|has|rename|kill
  lterm                  本地终端会话
```

终端支持命名会话、列出、重命名、结束，`new NAME --shell cmd` 可指定 shell。

## 从 .gv_pb 提取到的协议域（213 个消息）

- 画面采集：`CaptureConfig`、`CaptureParams`、`CaptureSettingRequest/Response/Sync`、`CaptureChange`、`Screen`、`ScreenResolution`、`ScreenType`、`BuiltinScreenType`、`VirtualDisplayMode`、`CreateVirtualDisplayRequest/Response`、`RemoveVirtualDisplayRequest/Response`、`EnterSuperScreenRequest/Response`、`QuitSuperScreenRequest/Response`
- 编码/画质：`CodecNegotiation`、`CodecType`、`ChromaFormat`、`FPS`、`FpsSettingRequest`、`ResolutionSettingRequest`、`VideoQuality`、`VideoQualitySettingReqeust`、`EncoderCapb`、`DecoderCap`
- 鼠标键盘/触摸：`InputEvent`、`TouchEvent`、`TouchEventPoint`、`VirtualMouseSwitchRequest/Response`、`CursorSettingReqeust`、`CursorShape`
- 剪贴板：`ClipboardRequest/Response`、`ClipboardChange`、`ClipboardDataBlock*`、`ClipboardFile*`、`ClipboardFormat*`
- 文件传输：`FileTransfer*`、`FileEntry`、`FileDirectory`、`ReadDir`、`FileRename`、`FileRemove*`、`FileDirCreate`、`DirectorySizeRead`、`DragDrop*`
- 端口映射：`PortMappingFrame`（`FrameType`）
- 远程开机：服务端字符串 `wake_on_lan`、`support_wol`、`router_wol`、`wol_guide`、`Server.WoL`
- 远程终端：`TerminalSessionChanged`、`RenameSessionReq/Resp`、`CT_TERMINAL`
- 隐私屏：`PrivateScreenSettingRequest/Response`、`PrivateScreenState`、`PrivateScreenType`
- 系统/权限：`PermissionState`、`PermissionAuthState`、`SecureDesktopState`、`QuerySystemState`、`SystemMetrics`、`ActiveWindowChange`、`ShowApp`、`LaunchApp`、`QuickLaunch*`
- 音频：`SystemSoundSupport`、`VirtualAudioDriverPolicyRequest/Response`
- 摄像头/USB/打印机：服务端字符串 `CameraInfo/CameraType/CameraID`、`usbhub/usbxhci`、`usbccgp`
- 多屏/超级屏：`EnterSuperScreenRequest`、`ScreenSources`、`SendFlipScreenRequest/Response`
- 手绘标注：`DrawRequest`、`DrawStrokeRequest`、`DrawClearRequest`、`DrawToggleRequest`
- 插件/ROM/Mumu：`Plugin*`、`RomMessage`、`Mumu*`
- RPC：`RpcRequest/Response`、`RequestHeader/ResponseHeader`

## CrossDesk 现状对照

| UU 能力 | CrossDesk 现状 | 本轮补齐 |
| --- | --- | --- |
| 端口映射 | 无 | 新增 `libs/portmap`，TCP 转发 + 单测通过；wire 增加 `port_mapping_open/close`；桌面被控端已接入 |
| 远程终端 | 有协议与进程宿主 | 控制端 UI/CLI 入口仍待做 |
| 远程开机 WOL | 有 `libs/wol` | 桌面 UI 入口与 MAC 上报仍待做 |
| 144fps / HDR / 4:4:4 | 仅 30/60、无 HDR | wire `VideoSettings` 增加 `frame_rate` 120/144、`hdr`、`chroma`，校验同步放宽 |
| 手柄 | 协议 + 桌面注入 | iOS `GCController` 采集、按键映射、按下/抬起状态已接入；Android JNI 手柄按钮/轴已接入 |
| 按键映射 | 桌面/Android 已生效 | iOS 映射引擎接入发送链路（键盘、手柄、宏、鼠标动作） |
| 文件/剪贴板/多屏 | 已有 | — |
| 隐私屏/可信设备 | 已有 | — |
| 账号/设备云同步 | 无 | 待做 |
| 手机副屏/超级屏 | 部分（PiP/虚拟显示） | 待做 |
| 摄像头/USB/打印机 | 无 | 待做 |
| 手绘标注 | 无 | 待做 |
| 云电脑 | 无 | 待做 |
| 插件/Mumu | 无 | 待做 |

## 结论

- 本机安装包可以稳定提取协议消息名、CLI 命令和模块边界，足以还原功能面，不需要反编译出源码。
- 三端功能还原的优先级：端口映射/终端/WOL → 高帧率画质 → 手柄与按键映射 → 账号云同步 → 副屏/摄像头/USB。
- 本轮已落地端口映射模块、iOS 映射与手柄采集、Android 手柄状态、视频设置扩展；其余按上表继续。
