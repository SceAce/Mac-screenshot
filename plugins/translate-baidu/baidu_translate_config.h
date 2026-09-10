#pragma once

#include <QString>

namespace markshot::translate_baidu {

/**
 * 百度翻译插件配置。
 *
 * endpoint 默认指向开放平台正式地址，仅在使用私有代理或测试替身时才需要覆盖。
 */
struct BaiduTranslateConfig {
    QString appId;
    QString appKey;
    QString endpoint = QStringLiteral("https://fanyi-api.baidu.com/api/trans/vip/translate");
    int timeoutMs = 30000;
};

/**
 * 读取百度翻译插件配置。
 * @return 合并 translation.baidu 子节、公共节、环境变量与默认值后的配置。
 */
BaiduTranslateConfig readBaiduTranslateConfig();

/**
 * 校验配置是否具备发起请求的必要字段。
 * @param config 待校验配置。
 * @param error 输出错误信息，指明缺失字段与可用的环境变量名。
 * @return 配置可用时返回 true。
 */
bool validateBaiduTranslateConfig(const BaiduTranslateConfig &config, QString *error);

}  // namespace markshot::translate_baidu
