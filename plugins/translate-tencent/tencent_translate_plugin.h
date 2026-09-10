#pragma once

#include "markshot/translate_provider_plugin.h"

#include <QObject>

namespace markshot::translate_tencent {

/**
 * 腾讯云机器翻译 provider 插件。
 *
 * 插件默认调用 TextTranslateBatch 一次翻译多段，批量接口不可用时在同一次
 * translate 调用内降级为逐段调用 TextTranslate。
 */
class TencentTranslatePlugin final : public QObject, public markshot::plugin::TranslateProviderPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID MARK_SHOT_TRANSLATE_PROVIDER_PLUGIN_IID FILE "metadata.json")
    Q_INTERFACES(markshot::plugin::TranslateProviderPlugin)

public:
    /**
     * 读取插件 provider 标识。
     * @return provider 标识，用于配置项选择插件。
     */
    QString providerId() const override;

    /**
     * 读取插件展示名称。
     * @return 面向用户显示的 provider 名称。
     */
    QString displayName() const override;

    /**
     * 检查腾讯云机器翻译配置是否可用。
     * @param error 输出错误信息。
     * @return 配置可用时返回 true。
     */
    bool isAvailable(QString *error) const override;

    /**
     * 调用腾讯云机器翻译接口翻译 OCR 分段。
     * @param segments 待翻译分段。
     * @param targetLanguage 目标语言。
     * @param translations 输出翻译后的分段，顺序与 id 同输入一致。
     * @param error 输出错误信息。
     * @return 翻译成功时返回 true。
     */
    bool translate(const QVector<markshot::plugin::TranslateSegment> &segments,
                   const QString &targetLanguage,
                   QVector<markshot::plugin::TranslateSegment> *translations,
                   QString *error) override;
};

}  // namespace markshot::translate_tencent
