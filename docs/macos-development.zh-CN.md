# macOS 开发与本地打包

当前首版支持 macOS 14 及以上。采用 Qt 6 Widgets 和 ScreenCaptureKit，提供
屏幕权限引导、截图选区、标注、复制、保存、贴图及菜单栏入口。
macOS 全局快捷键、登录启动、连续录屏和音频采集尚未适配。

## 构建（fish 可直接执行）

```fish
brew install cmake ninja qt pkgconf
# OCR、扫码的可选依赖
brew install onnxruntime zxing-cpp tesseract tesseract-lang

cmake -S . -B build-macos -G Ninja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_PREFIX_PATH=(brew --prefix qt) \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
    -DMARK_SHOT_WITH_LAYER_SHELL=OFF \
    -DBUILD_TESTING=ON
cmake --build build-macos --parallel 6
env QT_QPA_PLATFORM=offscreen ctest --test-dir build-macos --output-on-failure
python3 scripts/package-macos.py
open "dist/Mark Shot.app"
```

`build-macos/mark-shot.app` 是开发产物，仍引用构建机依赖。打包脚本将 Qt、扫码库
及常用图片格式插件的动态依赖复制到 `dist/Mark Shot.app`，检查外部动态库依赖，
并验证本地 ad-hoc 签名。复制该 `.app` 到其他目录后可以直接启动。
本地包没有 Developer ID 公证；对外发布时另行准备签名、公证和依赖许可材料。

默认打包基础截图版本。如需同时打包可选 OCR/翻译 provider 动态库，使用
`python3 scripts/package-macos.py --with-provider-plugins`。OCR 模型和外部
Tesseract 程序仍需单独准备；当前阶段不把它们作为基础截图包的运行依赖。

最低系统版本也受所用 Homebrew 依赖约束；这里只设置本项目为 14.0，旧系统需用
同样支持该系统的 Qt 和第三方库重新构建。当前测试目标为 Apple Silicon。

## 屏幕录制权限

1. 先确定应用存放位置。建议将最终 `.app` 放到固定目录后再授权。
2. 从 Finder 或 `open` 启动，首次截图会显示授权引导。
3. 点击“请求权限”，在“系统设置 → 隐私与安全性 → 屏幕与系统音频录制”中启用 Mark Shot。
   系统版本较旧时此页面名称为“屏幕录制”。也可使用引导中的“打开系统设置”。
4. 返回应用继续；若系统提示退出并重新打开应用，按系统提示操作。
5. 取消授权后不会生成桌面截图，可以通过菜单栏“屏幕录制权限”再次进入。

屏幕授权由 macOS TCC 管理，不能由应用自行开启。此阶段的截图和剪贴板功能
不需要麦克风、摄像头、辅助功能或完全磁盘访问权限。保存使用系统文件选择器。
不使用非标准的 `NSScreenCaptureUsageDescription` 键；实际授权通过
`CGPreflightScreenCaptureAccess` / `CGRequestScreenCaptureAccess` 和系统设置完成。

默认 bundle identifier 为 `io.github.scemac.MarkShot`。本地重新签名、改变标识或
移动应用后，系统可能要求重新授权。权限验证应以最终 `.app` 经 LaunchServices
启动的结果为准，终端直接执行内部程序时权限归属可能不同。

```fish
# 只读查询；已授权退出码 0，未授权退出码 1
"dist/Mark Shot.app/Contents/MacOS/mark-shot" --check-permissions
# 显示授权引导
open "dist/Mark Shot.app" --args --request-permissions
# 默认常驻菜单栏并开始一次截图；Esc 取消后可从菜单栏再次截图
open "dist/Mark Shot.app"
# 无界面截图验证，应在已授权后运行
open -n -W "dist/Mark Shot.app" --args --capture-to /tmp/mark-shot-smoke.png
```

## 实机验收

- 首次未授权、取消授权、系统设置中授权后重开、撤销授权后的错误提示。
- 单屏截图、矩形裁切、标注、撤销、保存 PNG、粘贴到预览或聊天软件。
- Retina 导出尺寸：100×100 逻辑点选区在 2 倍屏幕上应保留约 200×200 像素。
- 外接显示器、不同缩放、左侧/上方负坐标屏幕、跨屏截图。
- 截图覆盖层不进入新的全屏 Space；Esc 退出后菜单栏入口仍可截图。
- 贴图拖动、缩放和关闭；跨 Spaces 的贴图行为仍需后续专项验证。

组合截图使用最高屏幕比例建立画布，保留高分屏原始像素，低比例屏幕会放大到
同一画布比例。显示器之间的空白区域保持透明。自动测试覆盖 Retina 裁切、混合
缩放、负坐标和图像分配上限；权限弹窗及实际桌面内容仍需要实机验收。

图片复制使用 macOS 原生 `public.png`，同时提供 LZW 无损压缩 TIFF，避免
Qt 默认未压缩 TIFF 在长截图时膨胀为数十 MB。图片数据在窗口关闭前写入系统
剪贴板，应用退出后仍可粘贴。复制失败时保留截图，并显示“复制失败”。
`macos-clipboard` 测试使用独立命名剪贴板，验证普通图、长图、横向拼接图在
写入进程退出后仍可被原生 API 读取；需要在能访问 macOS 剪贴板服务的桌面会话运行。

## 2026-09-14 本机验证

- macOS 26.6.2 / Apple Silicon / Qt 6.11.2 编译成功。
- 69 个 CTest 项目全部通过；4 个本机 HTTP 模拟服务测试在沙箱外重跑通过。
- 基础 `.app` 约 81 MB，50 个 Mach-O 文件的非系统依赖均位于包内，严格签名验证通过。
- 清除 Qt/DYLD 外部搜索变量后启动成功；动态加载日志中没有 Homebrew 库路径。
- 未授权时返回中文错误且不生成截图；通过最终 `.app` 启动后已实际取得屏幕截图。
- 全屏 1728×1117 逻辑点输出 3456×2234 PNG；200×120 逻辑点区域输出 400×240 PNG。
- 交互截图使用 Cocoa 平台，覆盖窗口位于 `(0,0)`，尺寸 1728×1117，图像比例 2.0。

本机实测只有内建显示器。外接显示器、跨 Spaces，以及标注后的跨应用粘贴仍需要
进一步实机验证；混合缩放和负坐标目前由自动测试覆盖。
