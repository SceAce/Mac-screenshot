# Mark Shot for macOS

Mark Shot 是一个基于 Qt 6 的 macOS 截图和标注工具，支持区域截图、窗口自动吸附、标注、剪贴板、贴图悬浮窗口、Apple Vision OCR 和 OCR 文本翻译。

当前的项目基于[mark-shot](https://github.com/jswysnemc/mark-shot)二开,原功能[README](./README.zh-CN.md)；

当前 macOS 移植版本面向 macOS 14 及以上，CI 和发布包以 Apple Silicon（arm64）为主。Linux/Windows 的旧代码仍保留在仓库中，但 macOS 使用原生 ScreenCaptureKit、CoreGraphics 和 Apple Vision 实现截图、窗口检测和 OCR。

## 功能概览

- 截图时自动识别普通应用窗口，鼠标悬停显示窗口轮廓，单击即可选中窗口。
- 使用 Apple Vision 进行 OCR，不需要安装 Python、Tesseract 或 OCR 模型。
- 支持标注、裁切、复制、保存和置顶贴图。
- 翻译支持 OpenAI-compatible 中转站、腾讯、百度和有道 provider。
- 没有大模型 API Key 时，可以回退到 `trans`（translate-shell）；`trans` 会继承终端代理变量，并读取 macOS 系统代理配置。

## 系统要求

- macOS 14 Sonoma 或更高版本。
- Apple Silicon Mac（M1/M2/M3/M4 等）。
- 首次截图需要在“系统设置 → 隐私与安全性 → 屏幕与系统音频录制”中允许 Mark Shot。
- 发布包为本地 ad-hoc 签名，不包含 Apple Developer ID 公证。首次打开时如果出现安全提示，请在 Finder 中右键应用并选择“打开”。

## 方式一：下载预编译包

### 从 GitHub Releases 下载

打开 [GitHub Releases](https://github.com/SceAce/Mac-screenshot/releases)，下载对应版本的 `mark-shot-macos-arm64.dmg` 或 `mark-shot-macos-arm64.zip`。

如果该版本已上传 DMG，也可以在终端执行：

```bash
curl -L -o mark-shot-macos-arm64.dmg \
  https://github.com/SceAce/Mac-screenshot/releases/latest/download/mark-shot-macos-arm64.dmg

hdiutil attach mark-shot-macos-arm64.dmg
ditto "/Volumes/Mark Shot/Mark Shot.app" "/Applications/Mark Shot.app"
hdiutil detach "/Volumes/Mark Shot"
open "/Applications/Mark Shot.app"
```

### 从 GitHub Actions 下载

每次提交的 `Build` 工作流会生成 `mark-shot-macos-arm64` artifact，其中包含 DMG 和 ZIP。打开仓库的 **Actions → Build → 对应运行记录 → Artifacts** 下载即可。

也可以使用 GitHub CLI：

```bash
brew install gh
gh auth login
gh run list --workflow build.yml --limit 5
gh run download RUN_ID --name mark-shot-macos-arm64 --dir ./mark-shot-artifact
```

将 `RUN_ID` 替换为目标构建记录的编号。下载后可直接双击 DMG，或执行：

```bash
hdiutil attach ./mark-shot-artifact/mark-shot-macos-arm64.dmg
ditto "/Volumes/Mark Shot/Mark Shot.app" "/Applications/Mark Shot.app"
hdiutil detach "/Volumes/Mark Shot"
open "/Applications/Mark Shot.app"
```

## 方式二：从源码编译并安装

### 安装构建工具

先安装 Xcode Command Line Tools：

```bash
xcode-select --install
```

如果尚未安装 Homebrew，可执行：

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

安装 Mark Shot 的必需构建依赖：

```bash
brew install cmake ninja qt pkgconf python
```

核心截图、窗口吸附和 Apple Vision OCR 不需要额外的 OCR 程序。若希望在没有 API Key 时使用 `trans` 翻译，再安装：

```bash
brew install translate-shell
trans -V
```

可选的 RapidOCR 和二维码插件依赖如下；普通截图和 Apple Vision OCR 不需要它们：

```bash
brew install onnxruntime zxing-cpp
```

### 获取源码并编译

```bash
git clone https://github.com/SceAce/Mac-screenshot.git
cd mark-shot

cmake -S . -B build-macos -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qt)" \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
  -DMARK_SHOT_WITH_LAYER_SHELL=OFF \
  -DMARK_SHOT_WITH_LIBPORTAL=OFF

cmake --build build-macos --parallel
```

运行测试：

```bash
ctest --test-dir build-macos --output-on-failure
```

### 生成并安装 `.app`

直接运行 `build-macos/mark-shot.app` 只适合开发调试。要把 Qt 和动态库部署进可移动的应用包，请执行：

```bash
python3 scripts/package-macos.py \
  --build-dir build-macos \
  --output "dist/Mark Shot.app"

ditto "dist/Mark Shot.app" "/Applications/Mark Shot.app"
open "/Applications/Mark Shot.app"
```

打包脚本默认包含已经成功构建的 provider 插件。只需要基础截图功能时，可以跳过插件：

```bash
python3 scripts/package-macos.py \
  --build-dir build-macos \
  --without-provider-plugins \
  --output "dist/Mark Shot.app"
```

也可以将应用打成 DMG：

```bash
hdiutil create -volname "Mark Shot" \
  -srcfolder "dist/Mark Shot.app" \
  -ov -format UDZO mark-shot-macos-arm64.dmg
```

## 首次运行与权限

启动应用：

```bash
open "/Applications/Mark Shot.app"
```

首次截图时，系统会要求屏幕录制权限。打开“系统设置 → 隐私与安全性 → 屏幕与系统音频录制”，启用 **Mark Shot**，然后退出并重新打开应用。权限归属于应用包；移动应用、重新签名或更换 Bundle Identifier 后，macOS 可能要求重新授权。

检查权限而不启动界面：

```bash
"/Applications/Mark Shot.app/Contents/MacOS/mark-shot" --check-permissions
```

## OCR 与翻译

### OCR

macOS 的 `builtin` OCR 使用 Apple Vision：

- 不需要 Python、Tesseract、RapidOCR 或模型文件。
- 支持中英文等系统识别语言。
- OCR 结果会保留文字位置，可用于选中文字和翻译。

### 翻译

在设置中选择翻译 provider，或参考[翻译服务提供方文档](docs/translation-providers.zh-CN.md)编辑配置。OpenAI-compatible provider 的常用环境变量示例：

```bash
export MARK_SHOT_LLM_API_BASE="https://your-relay.example.com/v1"
export MARK_SHOT_LLM_API_KEY="your-api-key"
export MARK_SHOT_LLM_MODEL="your-model"
```

没有 API Key 时，`auto` 会尝试使用本机 `trans`：

```bash
brew install translate-shell
HTTP_PROXY=http://127.0.0.1:7897 \
HTTPS_PROXY=http://127.0.0.1:7897 \
ALL_PROXY=socks5://127.0.0.1:7897 \
  trans -no-ansi -brief -no-warn :zh "screenshot window"
```

应用启动的 `trans` 子进程会自动继承这些代理变量；从 Finder 启动时，还会读取 macOS 的 HTTP、HTTPS 和 SOCKS 系统代理设置。

## 常用命令

```bash
# 启动截图应用
open "/Applications/Mark Shot.app"

# 无界面截图（已授权时）
"/Applications/Mark Shot.app/Contents/MacOS/mark-shot" \
  --capture-to /tmp/mark-shot.png

# 检查版本
"/Applications/Mark Shot.app/Contents/MacOS/mark-shot" --version
```

更多配置、测试和打包说明见[macOS 开发说明](docs/macos-development.zh-CN.md)。

## 许可证

本项目基于 [MIT License](LICENSE) 开源。
