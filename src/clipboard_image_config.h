#pragma once

#include <QJsonObject>
#include <QtGlobal>

namespace markshot {

enum class ClipboardImageMode {
    ImagePng,
    Url,
    Threshold,
};

struct ClipboardImageConfig {
    ClipboardImageMode mode = ClipboardImageMode::ImagePng;
    int thresholdM = 4;
};

/**
 * 读取图片剪贴板配置。
 * @return 图片剪贴板配置，配置缺失或非法时返回默认配置。
 */
ClipboardImageConfig configuredClipboardImageConfig();

/**
 * 从配置根对象读取图片剪贴板配置。
 * @param root 应用配置根对象。
 * @return 图片剪贴板配置，配置缺失或非法时返回默认配置。
 */
ClipboardImageConfig clipboardImageConfigFromRoot(const QJsonObject &root);

/**
 * 计算图片剪贴板阈值字节数。
 * @param config 图片剪贴板配置。
 * @return 阈值对应的字节数。
 */
qsizetype clipboardImageThresholdBytes(const ClipboardImageConfig &config);

}  // namespace markshot
