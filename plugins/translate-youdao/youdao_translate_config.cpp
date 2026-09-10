#include "youdao_translate_config.h"

#include "translate_config_source.h"

#include <QStringList>

namespace markshot::translate_youdao {
namespace {

using markshot::translate_common::TranslateConfigSource;

// timeoutMs 的取值区间，避免配置写出无意义的超时
constexpr int kMinTimeoutMs = 1000;
constexpr int kMaxTimeoutMs = 300000;

/**
 * 读取 appKey 的环境变量候选名。
 * @return 按优先级排列的环境变量名列表。
 */
QStringList appKeyEnvNames()
{
    return {QStringLiteral("MARK_SHOT_YOUDAO_APP_KEY"), QStringLiteral("YOUDAO_APP_KEY")};
}

/**
 * 读取 appSecret 的环境变量候选名。
 * @return 按优先级排列的环境变量名列表。
 */
QStringList appSecretEnvNames()
{
    return {QStringLiteral("MARK_SHOT_YOUDAO_APP_SECRET"), QStringLiteral("YOUDAO_APP_SECRET")};
}

}  // namespace

YoudaoTranslateConfig readYoudaoTranslateConfig()
{
    const TranslateConfigSource source =
        markshot::translate_common::readTranslateConfigSource(QStringLiteral("youdao"));

    YoudaoTranslateConfig result;

    // 1. 凭据允许只放在环境变量里，避免密钥落到配置文件
    result.appKey =
        markshot::translate_common::configString(source, QStringLiteral("appKey"), appKeyEnvNames(), {});
    result.appSecret =
        markshot::translate_common::configString(source, QStringLiteral("appSecret"), appSecretEnvNames(), {});

    // 2. endpoint 仅支持配置文件覆盖，用于私有网关或测试桩
    result.endpoint =
        markshot::translate_common::configString(source, QStringLiteral("endpoint"), {}, result.endpoint);

    result.timeoutMs = markshot::translate_common::configInt(source,
                                                             QStringLiteral("timeoutMs"),
                                                             result.timeoutMs,
                                                             kMinTimeoutMs,
                                                             kMaxTimeoutMs);
    return result;
}

bool validateYoudaoTranslateConfig(const YoudaoTranslateConfig &config, QString *error)
{
    if (config.appKey.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("missing youdao appKey: set translation.youdao.appKey or %1")
                         .arg(appKeyEnvNames().join(QStringLiteral(" / ")));
        }
        return false;
    }
    if (config.appSecret.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("missing youdao appSecret: set translation.youdao.appSecret or %1")
                         .arg(appSecretEnvNames().join(QStringLiteral(" / ")));
        }
        return false;
    }
    if (config.endpoint.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("missing youdao endpoint: set translation.youdao.endpoint");
        }
        return false;
    }
    return true;
}

}  // namespace markshot::translate_youdao
