#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace markshot::translate_common {

/**
 * 翻译插件配置来源。
 *
 * translation 为应用配置中的公共翻译节，vendor 为其下的厂商子节。
 */
struct TranslateConfigSource {
    QJsonObject translation;
    QJsonObject vendor;
};

/**
 * 读取应用配置中的 translation 节。
 * @return translation JSON 对象，未找到配置文件时为空对象。
 */
QJsonObject readTranslationConfig();

/**
 * 读取指定厂商的翻译配置来源。
 * @param vendorKey 厂商子节名称，例如 tencent、baidu、youdao。
 * @return 公共节与厂商子节组成的配置来源。
 */
TranslateConfigSource readTranslateConfigSource(const QString &vendorKey);

/**
 * 按厂商节、公共节、环境变量、默认值的顺序读取字符串配置。
 * @param source 配置来源。
 * @param key 配置键名。
 * @param envNames 环境变量候选名，按顺序取首个非空值。
 * @param fallback 全部缺失时的默认值。
 * @return 去除首尾空白后的取值。
 */
QString configString(const TranslateConfigSource &source,
                     const QString &key,
                     const QStringList &envNames,
                     const QString &fallback = QString());

/**
 * 按厂商节、公共节的顺序读取整数配置。
 * @param source 配置来源。
 * @param key 配置键名。
 * @param fallback 缺失时的默认值。
 * @param minimum 允许的最小值。
 * @param maximum 允许的最大值。
 * @return 截断到取值区间后的整数。
 */
int configInt(const TranslateConfigSource &source,
              const QString &key,
              int fallback,
              int minimum,
              int maximum);

}  // namespace markshot::translate_common
