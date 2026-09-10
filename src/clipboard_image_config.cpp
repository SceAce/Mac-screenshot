#include "clipboard_image_config.h"

#include "app_config_store.h"
#include "config_value.h"

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include <algorithm>
#include <optional>

namespace {

constexpr int kDefaultThresholdM = 4;
constexpr int kMinThresholdM = 1;
constexpr int kMaxThresholdM = 1024;
constexpr qsizetype kBytesPerM = 1024 * 1024;

/**
 * 解析图片剪贴板模式。
 * @param value 配置中的模式值。
 * @return 解析成功时返回图片剪贴板模式，否则返回空值。
 */
std::optional<markshot::ClipboardImageMode> imageModeFromValue(const QJsonValue &value)
{
    if (!value.isString()) {
        return std::nullopt;
    }

    const QString mode = markshot::config::normalizedKey(value.toString());
    if (mode == QStringLiteral("imagepng")
        || mode == QStringLiteral("png")
        || mode == QStringLiteral("image")) {
        return markshot::ClipboardImageMode::ImagePng;
    }
    if (mode == QStringLiteral("url")
        || mode == QStringLiteral("file")
        || mode == QStringLiteral("fileurl")) {
        return markshot::ClipboardImageMode::Url;
    }
    if (mode == QStringLiteral("threshold")
        || mode == QStringLiteral("auto")
        || mode == QStringLiteral("size")) {
        return markshot::ClipboardImageMode::Threshold;
    }
    return std::nullopt;
}

/**
 * 读取图片剪贴板配置对象。
 * @param root 应用配置根对象。
 * @return 图片剪贴板配置对象。
 */
QJsonObject clipboardImageObject(const QJsonObject &root)
{
    // 1. 优先读取标准路径 clipboard.image
    const QJsonObject clipboard = markshot::config::firstObjectValue(root, QStringLiteral("clipboard"));
    const QJsonObject image = markshot::config::firstObjectValue(clipboard, QStringLiteral("image"));
    if (!image.isEmpty()) {
        return image;
    }

    // 2. 兼容扁平路径，便于手工配置
    return markshot::config::firstNonEmptyObjectValue(
        root,
        {QStringLiteral("clipboardImage"), QStringLiteral("imageClipboard")});
}

}  // namespace

namespace markshot {

ClipboardImageConfig configuredClipboardImageConfig()
{
    bool ok = false;
    const QJsonObject root = readAppConfigRoot(&ok);
    if (!ok) {
        return {};
    }

    return clipboardImageConfigFromRoot(root);
}

ClipboardImageConfig clipboardImageConfigFromRoot(const QJsonObject &root)
{
    ClipboardImageConfig config;
    config.thresholdM = kDefaultThresholdM;

    const QJsonObject image = clipboardImageObject(root);
    if (image.isEmpty()) {
        return config;
    }

    if (const std::optional<ClipboardImageMode> mode =
            imageModeFromValue(markshot::config::valueForKeys(
                image,
                {QStringLiteral("mode"),
                 QStringLiteral("policy"),
                 QStringLiteral("strategy")}))) {
        config.mode = *mode;
    }

    if (const std::optional<int> thresholdM =
            markshot::config::intValue(markshot::config::valueForKeys(
                image,
                {QStringLiteral("thresholdM"),
                 QStringLiteral("thresholdMB"),
                 QStringLiteral("thresholdMiB")}))) {
        config.thresholdM = std::clamp(*thresholdM, kMinThresholdM, kMaxThresholdM);
    }
    return config;
}

qsizetype clipboardImageThresholdBytes(const ClipboardImageConfig &config)
{
    const int thresholdM = std::clamp(config.thresholdM, kMinThresholdM, kMaxThresholdM);
    return static_cast<qsizetype>(thresholdM) * kBytesPerM;
}

}  // namespace markshot
