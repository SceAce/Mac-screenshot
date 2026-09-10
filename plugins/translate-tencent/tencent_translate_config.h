#pragma once

#include <QString>

namespace markshot::translate_tencent {

/**
 * 腾讯云机器翻译插件配置。
 *
 * 凭据既可写在 config.json 的 translation.tencent 节，也可只放在环境变量里。
 */
struct TencentTranslateConfig {
    QString secretId;
    QString secretKey;
    QString region = QStringLiteral("ap-guangzhou");
    QString endpoint = QStringLiteral("tmt.tencentcloudapi.com");
    int projectId = 0;
    int timeoutMs = 30000;
};

/**
 * 读取腾讯云机器翻译插件配置。
 * @return 合并厂商节、公共节、环境变量与默认值后的配置。
 */
TencentTranslateConfig readTencentTranslateConfig();

/**
 * 校验配置是否具备发起请求的必要字段。
 * @param config 待校验配置。
 * @param error 输出错误信息，指明缺失字段与可用的环境变量名。
 * @return 配置可用时返回 true。
 */
bool validateTencentTranslateConfig(const TencentTranslateConfig &config, QString *error);

}  // namespace markshot::translate_tencent
