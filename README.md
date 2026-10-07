# 三指拖拽 · Three-Finger Drag for Windows

[下载正式版 v1.0.0](https://github.com/aa1098823896-code/Three-Finger-Drag-Windows/releases/tag/v1.0.0) · [三指教学 / Tutorial](#tutorial) · [English instructions](#english)

轻量的 Windows 三指拖拽工具。三根手指轻触触控板并移动，即可移动窗口、选择文字、框选文件或桌面图标、框选截图区域、拖放文件与图片、调整窗口大小；**不需要重按触控板**，松手即结束。

移动沿用 Windows 的系统指针速度，不需要另调一套速度。常驻后台和设置界面分开，关闭设置窗口后仍可从托盘使用；只提供「三指拖拽」和「开机启动」两项日常设置。

<a name="tutorial"></a>

# 三指怎么用 · How to use

**轻放三指 → 一起滑动 → 抬手结束。不用重按触控板。**

![三指轻触教学：移动窗口、选择文字、框选图标、框选截图、拖放文件、调整大小](assets/tutorial.gif?v=1440-bilingual-2.5x)

中英双语演示依次展示六种常见操作，屏幕动作与手势同步。截图场景表示已经进入截图模式；在 Windows 中可以先按 `Win+Shift+S`。教学动画仅用于 README，不随软件运行。

## 轻触拖动，也轻装运行

原生 C + Win32 实现，不打包浏览器，不需要 .NET、Windows App SDK 或额外运行框架。关闭设置窗口后，只保留一个小型原生后台。

| 项目 | 体积或本机实测 |
| --- | --- |
| Windows x64 正式下载包 | 约 **108 KiB** |
| 两个程序合计 | 约 **246 KiB** |
| 常驻后台程序文件 | **39 KiB** |
| 后台空闲私有内存 | 约 **1.7 MiB** |
| 后台空闲工作集 | 约 **11.6 MiB** |
| 后台空闲 CPU | 10 秒采样未观察到 CPU 时间增长 |

内存和 CPU 数据来自一台 Windows 11 x64 电脑，只统计 `GestureHost.exe` 的空闲状态；设置窗口另计。工作集包含系统共享内存，私有内存是另一种统计口径，不能相加。实际占用随触控板、驱动、操作和系统状态变化。这些数字说明它很轻量，不表示零占用或完全没有性能影响。

## 下载安装

1. 在 [Releases](https://github.com/aa1098823896-code/Three-Finger-Drag-Windows/releases/latest) 下载 `three-finger-drag-v1.0.0-windows-x64.zip`，解压。
2. 在解压目录打开 PowerShell，运行下面这一行。安装脚本先核对文件 SHA-256，再安装到当前用户的程序目录，无需管理员权限。

   ```powershell
   powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Install.ps1
   ```

3. 安装后打开「三指拖拽」，首次使用完成下面两项系统设置。首次安装默认开机启动；更新时保留已有开关、开机启动和窗口位置。

也可以保持文件在一起，直接运行 `GestureSettings.exe` 使用便携版；启用开机启动后请勿随意移动目录。Windows 可能提示来自网络的脚本或未签名程序，请先核对下载来源和校验值；本项目没有付费签名证书。

安装包包含两个程序、共享图标、安装脚本、校验清单与许可。

## 交给自己的 Agent 安装

将下面这段话发送给**能够操作 Windows 电脑的 Agent（AI 助手）**即可。安装脚本负责安装和更新，Agent 负责确认设备和那两项系统设置：

> 请从 https://github.com/aa1098823896-code/Three-Finger-Drag-Windows/releases/latest 下载 Windows x64 正式安装包，核对 SHA256SUMS.txt，解压并检查包内 Install.ps1 后运行。确认这台电脑是 Windows 11 24H2 或更新的 x64 系统，有支持三指的精确触控板。在系统触控板设置中，把三指轻扫和三指点击设为“无”，已完成的就跳过。其他设置保持原样，不要关闭双击拖动，不要改指针速度。修改前记下原值，完成后重新打开软件检查状态，并让我试一次三指轻触拖动。若不能操作电脑，请直接说明，并教我最简单的手动步骤。

Agent 不需要安装额外框架，也不应为此修改全局脚本策略、重启电脑或结束无关程序。**安装成功不等于系统手势已经适配好**：那两项设置和一次实际拖动仍要确认。

## 首次设置：只改两项

需要 **Windows 精确触控板（Precision Touchpad）**，支持至少三指，且系统触控板开关已打开。当前发布包面向 Windows 11 24H2 及更新版本的 x64 系统，不提供 ARM64 包。

点击程序的「打开系统设置」，或进入 **设置 → 蓝牙和设备 → 触摸板 → 三指手势**：

- 将「轻扫」设为「无」。
- 将「点击」设为「无」。

这两项只需设置一次，用来避免 Windows 的应用切换、任务视图等动作与三指拖拽同时触发。**「双击并拖动以多选」可以保留开启**；无需调整单指速度、双指滚动、缩放或四指手势。

「打开系统设置」会打开相应页面，仍需在页面中设置。「交给 AI 办」会复制包含当前状态和具体步骤的提示词，请粘贴并发送给能够操作电脑的 AI；程序不会自行发送。不同 Windows 版本的状态检测存在兼容性差异，若手动设置完成后仍提示「需准备」，请反馈系统版本和触控板型号。

提示词用自然语言说明要改什么、哪些保持不变、改完怎么确认，以及不能操作电脑时怎么办。简体中文提示词不足 300 个字符；各语言都不插入本地用户名、电脑名或实际安装目录。软件无法读取设置时会如实说明，由 AI 在系统设置中确认。

## 怎么用

将指针移到窗口标题栏、可拖动区域或可选择的文字上。三根手指轻触触控板、一起滑动，抬手即可结束。不需要把触控板按下去。

- 右键蓝色托盘图标，可以切换拖拽、开机启动，打开设置或退出。
- 关闭设置窗口会保留托盘后台；从托盘「退出」才会结束整个程序。
- 多次打开设置会复用已有窗口。
- 设置窗口按屏幕 DPI 绘制，默认大小为 684 × 850 逻辑像素；拉大保持比例，工作区不足时等比例缩小。
- 遇到按键未释放，可用 `Ctrl+Alt+Pause` 暂停拖拽并释放按键。

更新时解压新包并运行 `Install.ps1`，脚本会正常关闭旧程序、备份原文件、核对替换结果并重新启动。备份放在安装目录的 `.previous/` 中，个人备份不需要上传到 GitHub。便携版手动更新时，先从托盘退出再替换文件。

设置保存于 `%LOCALAPPDATA%\ThreeFingerDragNative`。不再使用时，先关闭「开机启动」，再从托盘退出即可。

## 验证范围

v1.0.0 已在一台 Windows 11 25H2（26200.9168）电脑、两块不同 DPI 的屏幕上验证。用户实测确认：四角缩放和跨屏往返拖动正常；使用 `Win+Shift+S` 后三指框选只完成一次截图，`Esc` 可以正常取消；九种语言的排版已确认。

模拟回归覆盖正常抬指、混合触控报告、同一批触点尾帧防重复、复用触点 ID，以及连续 100 次手势。这些检查不注入真实输入，不能代替其他型号触控板的实际验证。目前没有覆盖所有 Windows 版本和设备。

教程手形已补齐五指，并修顺小拇指到掌缘的轮廓，保留原来的姿势、三指接触位置与画风。软件保留当前卡片布局。

## 界面语言

启动时跟随 Windows 显示语言，支持简体中文、繁体中文、英语、日语、韩语、德语、法语、西班牙语和葡萄牙语。其他显示语言使用英语；地区和键盘语言不会改变界面语言。程序没有额外的语言设置选项。

翻译维护在 `locales/`，构建时检查各语言的键与格式参数，并生成程序内置文字。语言匹配只在启动时读取一次。

## 目录结构

```text
src/            原生后台与设置界面源码
src/generated/  从共享素材与翻译生成的头文件
locales/        九种语言的可维护文字
tests/          不注入真实输入的回归测试
scripts/        素材、翻译和 README 教学动画的生成工具
assets/         共享图标、教程手形和 README 教学 GIF
docs/           原始许可、正式版本清单和教学 HTML
build.ps1       构建入口
test.ps1        测试入口
```

`build/` 保存本机生成的程序、预览和测试结果；编译器、个人诊断记录与备份不进入 Git。

## 从源码构建

使用 Windows x64 版 [Tiny C Compiler（TCC）](https://bellard.org/tcc/)。当前基线由 TCC 0.9.27 构建；编译器没有随仓库或发布包分发。

在仓库目录运行 PowerShell：

```powershell
.\build.ps1 -CompilerPath 'C:\tools\tcc\tcc.exe'
```

脚本在 `build/` 生成 `GestureHost.exe` 和 `GestureSettings.exe`，复制 `assets/Gesture.ico`，写入程序图标及版本信息，并运行手势、提示词隐私、窗口缩放及语言匹配回归。构建前请先退出从该构建目录运行的程序。

教程手形共用 `assets/tutorial-hand.svg`。构建脚本将其转换为 `src/generated/tutorial_hand.h` 中的原生矢量路径，两张教程卡片复用同一轮廓；修改手形时只需编辑这个 SVG。

README 教学动画来自 `docs/tutorial.html`，复用同一五指轮廓。文字使用自然字宽，高亮范围由文字实际宽度计算，不拉伸字形。需要重新导出时，使用 Node.js + Playwright 运行 `scripts/export-tutorial-gif.cjs`，再用 Python + Pillow + NumPy 运行 `scripts/encode-tutorial-gif.py`；这些依赖只用于文档导出，用户安装软件不需要它们。

默认导出宽 1440 像素、高度等比计算的中英双语动画，速度为 2.5 倍；连续动作每秒 50 帧，静止片段合并帧。本次导出使用本机已安装的华康圆体和 Segoe UI，仓库不包含字体文件。

只运行回归，或导出原生界面预览：

```powershell
.\test.ps1 -CompilerPath 'C:\tools\tcc\tcc.exe'
.\build.ps1 -CompilerPath 'C:\tools\tcc\tcc.exe' -RenderPreview
```

测试结果写入 `build/tests/`；预览是 `build/ui-preview.png`。这些开发产物不进入 Git。

## 版本与回退

`main` 保存已验收的代码；`v1.0.0` 是首个正式公开版本。开发期的两个版本标记已撤下。每项后续修改在独立分支完成，验收后再合并。

查看旧版而保留当前目录，可以创建另一个检出目录：

```powershell
git worktree add --detach ..\three-finger-drag-v1.0.0 v1.0.0
```

需要回到正式版时，可以使用对应的 Release 包。`docs/release-manifest.json` 记录正式包的程序、图标和安装脚本 SHA-256；Release 的 `SHA256SUMS.txt` 用于核对下载压缩包。

## 许可与来源

采用 [MIT License](LICENSE)。手势识别参考 [ThreeFingerDragOnWindows 2.0.7](https://github.com/ClementGre/ThreeFingerDragOnWindows/tree/2.0.7)，保留 Clément Grennerat 的版权和[原始许可](docs/LICENSE.upstream.txt)。本仓库包含原生后台、Win32 设置界面及相应修改；没有分发原版应用安装包或编译器。

## English

[↑ Watch the bilingual tutorial](#tutorial)

Lightweight three-finger drag for Windows precision touchpads. Move windows, select text, box-select files or screenshot areas, drag and drop, and resize windows. **Rest three fingers and slide; you do not need to press down.** Lift your fingers to finish. Movement follows the Windows pointer speed.

### Small by design

Native C + Win32, with no bundled browser or additional runtime. The Windows x64 download is about **108 KiB**; both executables total about **246 KiB**, and the resident host executable is **39 KiB**. Closing the settings window leaves only the native host running.

On one Windows 11 x64 computer, the idle host used about **1.7 MiB of private memory** and **11.6 MiB of working set**. A 10-second idle sample recorded no increase in CPU time. These are separate memory measurements for `GestureHost.exe`, excluding the settings window; usage varies with hardware, drivers and activity. They do not imply zero resource use. The animated guide above is documentation only and adds no runtime workload to the utility.

### Install

Windows 11 24H2 or later, x64, and a precision touchpad supporting at least three fingers are required. ARM64 is not supported by this package.

Download `three-finger-drag-v1.0.0-windows-x64.zip` from [Releases](https://github.com/aa1098823896-code/Three-Finger-Drag-Windows/releases/latest), verify `SHA256SUMS.txt`, and extract it. Review `Install.ps1`, then run this command from the extracted folder:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Install.ps1
```

The script verifies the files and installs for the current user without administrator privileges. A first installation enables startup at sign-in. Updates preserve your settings and back up the old files. You can also run `GestureSettings.exe` directly as a portable app. No extra runtime is required. The executables are not code-signed.

### Let your Agent handle setup

Send this to an Agent that can operate your Windows computer:

> Download the latest Windows x64 release from https://github.com/aa1098823896-code/Three-Finger-Drag-Windows/releases/latest, verify SHA256SUMS.txt, extract it, review Install.ps1, and run it. Confirm Windows 11 24H2 or later and a precision touchpad supporting three fingers. In Windows touchpad settings, set three-finger Swipes and Taps to Nothing, skipping settings already correct. Preserve all other settings, especially double-tap dragging and pointer speed. Record the previous values, reopen the app to check its status, and ask me to try a light three-finger drag. If you cannot operate my computer, tell me and give me the shortest manual steps.

Installation does not automatically change Windows gestures. Manually, go to **Settings → Bluetooth & devices → Touchpad → Three-finger gestures**, and set both **Swipes** and **Taps** to **Nothing**. Do not disable double-tap dragging. The app's **Ask AI** button copies setup instructions; paste them into an AI chat and send them yourself.

### Language and use

The app reads your Windows display language at startup: Simplified Chinese, Traditional Chinese, English, Japanese, Korean, German, French, Spanish, and Portuguese. Other languages use English. There is no language selector. After changing the Windows display language, restart the app. Region and keyboard settings do not select the UI language. This README offers Chinese and English instructions; GitHub does not select a translated README for you.

Closing the settings window keeps the tray app running. Right-click its blue icon to toggle dragging, change startup behavior, open settings, or quit. To recover from a stuck gesture, press `Ctrl+Alt+Pause`; it pauses dragging and releases held input.

This release was tested on one Windows 11 computer with two monitors at different DPI settings. Other touchpad models still need real-device validation. Report problems with your Windows version and touchpad model; do not include your private installation backup or personal paths.
