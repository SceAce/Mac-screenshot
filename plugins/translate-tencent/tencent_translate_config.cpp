#include "tencent_translate_config.h"

#include "translate_config_source.h"

#include <limits>

namespace markshot::translate_tencent {
namespace {

using markshot::translate_common::configInt;
using markshot::translate_common::configString;
using markshot::translate_common::readTranslateConfigSource;
using markshot::translate_common::TranslateConfigSource;

/** 配置文件中的厂商子节名称。 */
const QString kVendorKey = QStringLiteral("tencent");

/**
 * 读取 secretId 的环境变量候选名。
 * @return 按优先级排列的变量名列表。
 */
QStringList secretIdEnvNames()
{
    return {QStringLiteral("TENCENTCLOUD_SECRET_ID"), QStringLiteral("MARK_SHOT_TENCENT_SECRET_ID")};
}

/**
 * 读取 secretKey 的环境变量候选名。
 * @return 按优先级排列的变量名列表。
 */
QStringList secretKeyEnvNames()
{
    return {QStringLiteral("TENCENTCLOUD_SECRET_KEY"), QStringLiteral("MARK_SHOT_TENCENT_SECRET_KEY")};
}

/**
 * 读取 region 的环境变量候选名。
 * @return 按优先级排列的变量名列表。
 */
QStringList regionEnvNames()
{
    return {QStringLiteral("TENCENTCLOUD_REGION"), QStringLiteral("MARK_SHOT_TENCENT_REGION")};
}

/**
 * 拼接缺失字段的错误信息。
 * @param field 配置字段名。
 * @param envNames 可用的环境变量候选名。
 * @return 指明配置项与环境变量的错误信息。
 */
QString missingFieldError(const QString &field, const QStringList &envNames)
{
    QString message = QStringLiteral("missing tencent %1: set translation.tencent.%1").arg(field);
    if (!envNames.isEmpty()) {
        message += QStringLiteral(" or one of %1").arg(envNames.join(QStringLiteral(", ")));
    }
    return message;
}

}  // namespace

TencentTranslateConfig readTencentTranslateConfig()
{
    const TranslateConfigSource source = readTranslateConfigSource(kVendorKey);

    TencentTranslateConfig config;
    config.secretId = configString(source, QStringLiteral("secretId"), secretIdEnvNames());
    config.secretKey = configString(source, QStringLiteral("secretKey"), secretKeyEnvNames());
    config.region = configString(source, QStringLiteral("region"), regionEnvNames(), config.region);

    // 1. 接入点只从配置读取，测试与私有化部署可指向自建地址
    config.endpoint = configString(source, QStringLiteral("endpoint"), {}, config.endpoint);
    config.projectId = configInt(source,
                                 QStringLiteral("projectId"),
                                 config.projectId,
                                 0,
                                 std::numeric_limits<int>::max());
    config.timeoutMs = configInt(source, QStringLiteral("timeoutMs"), config.timeoutMs, 1000, 300000);
    return config;
}

bool validateTencentTranslateConfig(const TencentTranslateConfig &config, QString *error)
{
    // 1. 凭据缺失最常见，错误信息要同时给出配置路径与环境变量名
    if (config.secretId.trimmed().isEmpty()) {
        if (error) {
            *error = missingFieldError(QStringLiteral("secretId"), secretIdEnvNames());
        }
        return false;
    }
    if (config.secretKey.trimmed().isEmpty()) {
        if (error) {
            *error = missingFieldError(QStringLiteral("secretKey"), secretKeyEnvNames());
        }
        return false;
    }

    // 2. 接入点与地域参与签名和路由，为空时请求必然被拒
    if (config.endpoint.trimmed().isEmpty()) {
        if (error) {
            *error = missingFieldError(QStringLiteral("endpoint"), {});
        }
        return false;
    }
    if (config.region.trimmed().isEmpty()) {
        if (error) {
            *error = missingFieldError(QStringLiteral("region"), regionEnvNames());
        }
        return false;
    }
    return true;
}

}  // namespace markshot::translate_tencent
