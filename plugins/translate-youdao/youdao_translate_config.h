#pragma once

#include <QString>

namespace markshot::translate_youdao {

/**
 * 有道智云文本翻译插件配置。
 */
struct YoudaoTranslateConfig {
    QString appKey;
    QString appSecret;
    QString endpoint = QStringLiteral("https://openapi.youdao.com/api");
    int timeoutMs = 30000;
};

/**
 * 读取有道翻译插件配置。
 *
 * 取值优先级为 translation.youdao 子节、translation 公共节、环境变量、默认值。
 *
 * @return 合并配置文件、环境变量与默认值后的配置。
 */
YoudaoTranslateConfig readYoudaoTranslateConfig();

/**
 * 校验配置是否具备发起请求的必要字段。
 * @param config 待校验配置。
 * @param error 输出错误信息，指明缺失字段与可用的环境变量名。
 * @return 配置可用时返回 true。
 */
bool validateYoudaoTranslateConfig(const YoudaoTranslateConfig &config, QString *error);

}  // namespace markshot::translate_youdao
