#!/usr/bin/env bash

# 脚本功能：引导用户通过 GitHub CLI (gh) 提交 Issue，支持中英双语选择并自动收集系统环境信息

# 检查 gh 工具是否安装
if ! command -v gh &> /dev/null; then
    echo "未检测到 gh (GitHub CLI) 工具。请先安装。安装方法可参考: https://cli.github.com"
    exit 1
fi

# 检查 gh 登录状态
if ! gh auth status &> /dev/null; then
    echo "gh 工具未登录。请先运行 'gh auth login' 登录您的 GitHub 账号。"
    exit 1
fi

# 获取当前项目根目录
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# 选择语言
echo "Please choose your language / 请选择语言:"
echo "1) 中文 (Chinese)"
echo "2) English"
read -r -p "Enter number / 输入序号 (1 or 2): " lang_choice

if [ "$lang_choice" = "1" ]; then
    # 中文逻辑
    echo "请选择提交的 Issue 类型:"
    echo "1) Bug Report (Bug 报告，自动收集环境信息)"
    echo "2) Feature Request (新功能请求)"
    read -r -p "请输入序号 (1 或 2): " choice

    if [ "$choice" = "1" ]; then
        TEMPLATE_FILE="$REPO_DIR/.github/ISSUE_TEMPLATE/bug_report_zh.md"
        if [ ! -f "$TEMPLATE_FILE" ]; then
            echo "未找到中文 Bug 报告模板文件"
            exit 1
        fi

        echo "正在收集系统环境信息..."
        
        OS_NAME="未知"
        if [ -f /etc/os-release ]; then
            OS_NAME=$(grep -oP '(?<=^PRETTY_NAME=")[^"]+' /etc/os-release || grep -oP '(?<=^PRETTY_NAME=)[^[:space:]]+' /etc/os-release)
        fi

        DESKTOP_ENV="${XDG_CURRENT_DESKTOP:-未知}"
        DISPLAY_TYPE="${XDG_SESSION_TYPE:-未知}"
        
        MS_VERSION="未知 (未编译或未安装)"
        if command -v mark-shot &> /dev/null; then
            MS_VERSION=$(mark-shot --version 2>&1)
        elif [ -f "$REPO_DIR/build/mark-shot" ]; then
            MS_VERSION=$("$REPO_DIR/build/mark-shot" --version 2>&1)
        fi

        QT_VERSION="未知"
        if command -v qmake6 &> /dev/null; then
            QT_VERSION=$(qmake6 --version | grep -oP '(?<=Qt version )[0-9.]+')
        elif command -v qmake &> /dev/null; then
            QT_VERSION=$(qmake --version | grep -oP '(?<=Qt version )[0-9.]+')
        fi

        TEMP_ISSUE=$(mktemp)
        
        sed \
            -e "s/- Mark Shot 版本:.*/- Mark Shot 版本: $MS_VERSION/" \
            -e "s/- 操作系统 (OS):.*/- 操作系统 (OS): $OS_NAME/" \
            -e "s/- 窗口管理器 (Window Manager,.*) \/ 桌面环境 (Desktop,.*):.*/- 窗口管理器 (Window Manager) \/ 桌面环境 (Desktop): $DESKTOP_ENV/" \
            -e "s/- 显示服务器类型 (Display Server):.*/- 显示服务器类型 (Display Server): $DISPLAY_TYPE/" \
            -e "s/- Qt 版本 (Qt Version):.*/- Qt 版本 (Qt Version): $QT_VERSION/" \
            "$TEMPLATE_FILE" > "$TEMP_ISSUE"

        echo "环境信息收集完毕。正在启动 gh 创建 Bug 报告..."
        gh issue create --body-file "$TEMP_ISSUE" --label "bug"
        rm -f "$TEMP_ISSUE"

    elif [ "$choice" = "2" ]; then
        TEMPLATE_FILE="$REPO_DIR/.github/ISSUE_TEMPLATE/feature_request_zh.md"
        if [ ! -f "$TEMPLATE_FILE" ]; then
            echo "未找到中文新功能请求模板文件"
            exit 1
        fi
        echo "正在启动 gh 创建新功能请求..."
        gh issue create --body-file "$TEMPLATE_FILE" --label "enhancement"
    else
        echo "输入无效。已取消操作。"
        exit 1
    fi

elif [ "$lang_choice" = "2" ]; then
    # English Logic
    echo "Please select issue type:"
    echo "1) Bug Report (Bug Report, automatically collect environment info)"
    echo "2) Feature Request (Feature Request)"
    read -r -p "Enter number (1 or 2): " choice

    if [ "$choice" = "1" ]; then
        TEMPLATE_FILE="$REPO_DIR/.github/ISSUE_TEMPLATE/bug_report_en.md"
        if [ ! -f "$TEMPLATE_FILE" ]; then
            echo "English Bug Report template file not found"
            exit 1
        fi

        echo "Collecting system environment information..."
        
        OS_NAME="Unknown"
        if [ -f /etc/os-release ]; then
            OS_NAME=$(grep -oP '(?<=^PRETTY_NAME=")[^"]+' /etc/os-release || grep -oP '(?<=^PRETTY_NAME=)[^[:space:]]+' /etc/os-release)
        fi

        DESKTOP_ENV="${XDG_CURRENT_DESKTOP:-Unknown}"
        DISPLAY_TYPE="${XDG_SESSION_TYPE:-Unknown}"
        
        MS_VERSION="Unknown (Not compiled or installed)"
        if command -v mark-shot &> /dev/null; then
            MS_VERSION=$(mark-shot --version 2>&1)
        elif [ -f "$REPO_DIR/build/mark-shot" ]; then
            MS_VERSION=$("$REPO_DIR/build/mark-shot" --version 2>&1)
        fi

        QT_VERSION="Unknown"
        if command -v qmake6 &> /dev/null; then
            QT_VERSION=$(qmake6 --version | grep -oP '(?<=Qt version )[0-9.]+')
        elif command -v qmake &> /dev/null; then
            QT_VERSION=$(qmake --version | grep -oP '(?<=Qt version )[0-9.]+')
        fi

        TEMP_ISSUE=$(mktemp)
        
        sed \
            -e "s/- Mark Shot Version:.*/- Mark Shot Version: $MS_VERSION/" \
            -e "s/- Operating System (OS):.*/- Operating System (OS): $OS_NAME/" \
            -e "s/- Window Manager (e.g. niri, sway) \/ Desktop Environment (e.g. GNOME):.*/- Window Manager (e.g. niri, sway) \/ Desktop Environment (e.g. GNOME): $DESKTOP_ENV/" \
            -e "s/- Display Server Type:.*/- Display Server Type: $DISPLAY_TYPE/" \
            -e "s/- Qt Version:.*/- Qt Version: $QT_VERSION/" \
            "$TEMPLATE_FILE" > "$TEMP_ISSUE"

        echo "Environment information collected. Launching gh to create Bug Report..."
        gh issue create --body-file "$TEMP_ISSUE" --label "bug"
        rm -f "$TEMP_ISSUE"

    elif [ "$choice" = "2" ]; then
        TEMPLATE_FILE="$REPO_DIR/.github/ISSUE_TEMPLATE/feature_request_en.md"
        if [ ! -f "$TEMPLATE_FILE" ]; then
            echo "English Feature Request template file not found"
            exit 1
        fi
        echo "Launching gh to create Feature Request..."
        gh issue create --body-file "$TEMPLATE_FILE" --label "enhancement"
    else
        echo "Invalid input. Operation cancelled."
        exit 1
    fi
else
    echo "Invalid choice. Cancelled. / 输入无效，操作取消。"
    exit 1
fi
