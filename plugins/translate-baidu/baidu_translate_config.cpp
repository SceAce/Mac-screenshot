#include "baidu_translate_config.h"

#include "translate_config_source.h"

#include <QStringList>

namespace markshot::translate_baidu {
namespace {

using markshot::translate_common::TranslateConfigSource;

/**
 * 读取 APPID 的环境变量候选名。
 * @return 按优先级排列的环境变量名列表。
 */
QStringList appIdEnvNames()
{
    return {QStringLiteral("MARK_SHOT_BAIDU_APP_ID"), QStringLiteral("BAIDU_TRANSLATE_APP_ID")};
}

/**
 * 读取密钥的环境变量候选名。
 * @return 按优先级排列的环境变量名列表。
 */
QStringList appKeyEnvNames()
{
    return {QStringLiteral("MARK_SHOT_BAIDU_APP_KEY"), QStringLiteral("BAIDU_TRANSLATE_APP_KEY")};
}

}  // namespace

BaiduTranslateConfig readBaiduTranslateConfig()
{
    const TranslateConfigSource source =
        markshot::translate_common::readTranslateConfigSource(QStringLiteral("baidu"));

    BaiduTranslateConfig result;

    // 1. 凭据可写在配置文件的 baidu 子节，也可只放在环境变量里避免落盘
    result.appId =
        markshot::translate_common::configString(source, QStringLiteral("appId"), appIdEnvNames());
    result.appKey =
        markshot::translate_common::configString(source, QStringLiteral("appKey"), appKeyEnvNames());

    // 2. endpoint 不提供环境变量候选，避免误改导致凭据被发往非官方地址
    result.endpoint = markshot::translate_common::configString(source,
                                                               QStringLiteral("endpoint"),
                                                               {},
                                                               result.endpoint);

    // 3. 超时截断到 1 秒至 5 分钟，防止配置写错让请求长时间悬挂
    result.timeoutMs = markshot::translate_common::configInt(source,
                                                             QStringLiteral("timeoutMs"),
                                                             result.timeoutMs,
                                                             1000,
                                                             300000);
    return result;
}

bool validateBaiduTranslateConfig(const BaiduTranslateConfig &config, QString *error)
{
    if (config.appId.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("missing baidu appId: set translation.baidu.appId or one of "
                                    "MARK_SHOT_BAIDU_APP_ID, BAIDU_TRANSLATE_APP_ID");
        }
        return false;
    }
    if (config.appKey.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("missing baidu appKey: set translation.baidu.appKey or one of "
                                    "MARK_SHOT_BAIDU_APP_KEY, BAIDU_TRANSLATE_APP_KEY");
        }
        return false;
    }
    if (config.endpoint.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("missing baidu endpoint: set translation.baidu.endpoint");
        }
        return false;
    }
    return true;
}

}  // namespace markshot::translate_baidu
