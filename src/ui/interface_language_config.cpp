#include "ui/interface_language_config.h"

#include <QJsonValue>
#include <QLocale>

namespace markshot::ui {

UiLanguageMode uiLanguageModeFromString(const QString &raw)
{
    const QString value = raw.trimmed().toLower();
    if (value == QStringLiteral("english") || value == QStringLiteral("en")
        || value == QStringLiteral("en_us") || value == QStringLiteral("en-us")) {
        return UiLanguageMode::English;
    }
    if (value == QStringLiteral("chinese") || value == QStringLiteral("zh")
        || value == QStringLiteral("zh_cn") || value == QStringLiteral("zh-cn")
        || value == QStringLiteral("cn")) {
        return UiLanguageMode::Chinese;
    }
    return UiLanguageMode::System;
}

QString uiLanguageModeName(UiLanguageMode mode)
{
    switch (mode) {
    case UiLanguageMode::English:
        return QStringLiteral("english");
    case UiLanguageMode::Chinese:
        return QStringLiteral("chinese");
    case UiLanguageMode::System:
        break;
    }
    return QStringLiteral("system");
}

UiLanguageMode uiLanguageModeFromConfigRoot(const QJsonObject &root)
{
    const QJsonValue uiValue = root.value(QStringLiteral("ui"));
    if (uiValue.isObject()) {
        const QString value = uiValue.toObject().value(QStringLiteral("language")).toString();
        if (!value.trimmed().isEmpty()) {
            return uiLanguageModeFromString(value);
        }
    }

    const QString legacyValue = root.value(QStringLiteral("language")).toString();
    return uiLanguageModeFromString(legacyValue);
}

markshot::i18n::Language languageForUiLanguageMode(UiLanguageMode mode)
{
    if (mode == UiLanguageMode::English) {
        return markshot::i18n::Language::English;
    }
    if (mode == UiLanguageMode::Chinese) {
        return markshot::i18n::Language::Chinese;
    }
    return QLocale::system().language() == QLocale::Chinese
        ? markshot::i18n::Language::Chinese
        : markshot::i18n::Language::English;
}

}  // namespace markshot::ui
