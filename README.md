# 三指拖拽 · Three-Finger Drag for Windows

发布准备中：[GitHub 仓库](https://github.com/aa1098823896-code/Three-Finger-Drag-Windows)目前为私有仓库，用于保存源码与版本记录。程序包尚未上传或对外发布。

轻量的 Windows 三指拖拽工具。三根手指轻触触控板并移动，即可移动窗口、选择文字、框选文件或桌面图标、框选截图区域、拖放文件与图片、调整窗口大小；**不需要重按触控板**，松手即结束。

移动沿用 Windows 的系统指针速度，不需要另调一套速度。常驻后台和设置界面分开，关闭设置窗口后仍可从托盘使用；只提供「三指拖拽」和「开机启动」两项日常设置。

## 下载安装

1. 正式发布后，在本仓库的 [Releases](https://github.com/aa1098823896-code/Three-Finger-Drag-Windows/releases) 下载 `three-finger-drag-v0.1.1-windows-x64.zip`。
2. 解压到一个固定目录，保持所有文件在一起。开机启动会使用该目录，启用后请勿随意移动。
3. 双击 `GestureSettings.exe`，打开「三指拖拽」。首次使用完成下面两项系统设置。

这是便携版，不需要 .NET、Windows App SDK 或额外运行框架。尚未提供安装向导。两个程序合计约 182 KiB；这表示文件体积，不代表内存占用。

## 首次设置：只改两项

需要 **Windows 精确触控板（Precision Touchpad）**，支持至少三指，且系统触控板开关已打开。当前发布包面向 Windows 11 x64。

点击程序的「打开系统设置」，或进入 **设置 → 蓝牙和设备 → 触摸板 → 三指手势**：

- 将「轻扫」设为「无」。
- 将「点击」设为「无」。

这两项只需设置一次，用来避免 Windows 的应用切换、任务视图等动作与三指拖拽同时触发。**「双击并拖动以多选」可以保留开启**；无需调整单指速度、双指滚动、缩放或四指手势。

「打开系统设置」会打开相应页面，仍需在页面中设置。「交给 AI 办」会复制包含当前状态和具体步骤的提示词，请粘贴并发送给能够操作电脑的 AI；程序不会自行发送。不同 Windows 版本的状态检测存在兼容性差异，若手动设置完成后仍提示「需准备」，请反馈系统版本和触控板型号。

提示词用几句自然语言说明要改什么、哪些保持不变、改完怎么确认，以及不能操作电脑时怎么办；不足 300 个字符，不插入本地用户名、电脑名或实际安装目录。软件无法读取设置时会如实说明，由 AI 在系统设置中确认。

## 怎么用

将指针移到窗口标题栏、可拖动区域或可选择的文字上。三根手指轻触触控板、一起滑动，抬手即可结束。不需要把触控板按下去。

- 右键蓝色托盘图标，可以切换拖拽、开机启动，打开设置或退出。
- 关闭设置窗口会保留托盘后台；从托盘「退出」才会结束整个程序。
- 多次打开设置会复用已有窗口。
- 设置窗口按屏幕 DPI 绘制，默认大小为 684 × 850 逻辑像素；拉大保持比例，工作区不足时等比例缩小。
- 遇到按键未释放，可用 `Ctrl+Alt+Pause` 暂停拖拽并释放按键。

更新或回退版本时，先从托盘退出，再替换程序文件并重新打开设置。如果换了存放目录，请在新目录启动后重新启用「开机启动」。

设置保存于 `%LOCALAPPDATA%\ThreeFingerDragNative`。不再使用时，先关闭「开机启动」，再从托盘退出即可。

## 验证范围

v0.1.0 已在一台 Windows 11 25H2（26200.9168）电脑、两块不同 DPI 的屏幕上验证。用户实测确认：使用 `Win+Shift+S` 后三指框选只完成一次截图，`Esc` 可以正常取消。

模拟回归覆盖正常抬指、混合触控报告、同一批触点尾帧防重复、复用触点 ID，以及连续 100 次手势。这些检查不注入真实输入，不能代替其他型号触控板的实际验证。目前没有覆盖所有 Windows 版本和设备。

当前教程插画和底部提示布局保留作为后续改进的基线。修正插画、动态提示及教程动效会分开迭代。

## 从源码构建

使用 Windows x64 版 [Tiny C Compiler（TCC）](https://bellard.org/tcc/)。当前基线由 TCC 0.9.27 构建；编译器没有随仓库或发布包分发。

在仓库目录运行 PowerShell：

```powershell
.\build.ps1 -CompilerPath 'C:\tools\tcc\tcc.exe'
```

脚本生成 `GestureHost.exe` 和 `GestureSettings.exe`，复用仓库中的 `Gesture.ico`，写入程序图标及版本信息，并运行两组手势模拟回归及提示词隐私回归。构建前请先退出从当前目录运行的程序。

只运行回归，或导出原生界面预览：

```powershell
.\test.ps1 -CompilerPath 'C:\tools\tcc\tcc.exe'
.\build.ps1 -CompilerPath 'C:\tools\tcc\tcc.exe' -RenderPreview
```

测试结果写入 `.test-output`；预览是 `ui-preview.png`。这些开发产物不进入 Git。

## 版本与回退

`main` 保存已验收的代码，`v0.1.0` 保留原始固定基线，`v0.1.1` 移除了 AI 提示词中的个人路径，是当前待发布版本。每项后续修改在独立分支完成，验收后再合并。

查看旧版而保留当前目录，可以创建另一个检出目录：

```powershell
git worktree add --detach ..\three-finger-drag-v0.1.0 v0.1.0
```

需要直接回到已验证的运行程序时，使用对应版本的 Release 包。`release-manifest.json` 记录该包中程序和图标的 SHA-256。

## 许可与来源

采用 [MIT License](LICENSE)。手势识别参考 [ThreeFingerDragOnWindows 2.0.7](https://github.com/ClementGre/ThreeFingerDragOnWindows/tree/2.0.7)，保留 Clément Grennerat 的版权和原始许可于 `LICENSE.upstream.txt`。本仓库包含原生后台、Win32 设置界面及相应修改；没有分发原版应用安装包或编译器。
