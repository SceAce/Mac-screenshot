#include "translate_language_code.h"

#include <QHash>

namespace markshot::translate_common {
namespace {

/**
 * 归一化用户输入，去除大小写、分隔符与空白差异。
 * @param language 目标语言名称。
 * @return 归一化后的查表键。
 */
QString normalizedInput(const QString &language)
{
    QString key;
    key.reserve(language.size());
    for (const QChar character : language) {
        if (character.isSpace() || character == QLatin1Char('_') || character == QLatin1Char('-')) {
            continue;
        }
        key.append(character.toLower());
    }
    return key;
}

/**
 * 读取语言别名到规范键的映射表。
 * @return 别名映射表，键为归一化后的别名。
 */
const QHash<QString, QString> &languageAliases()
{
    static const QHash<QString, QString> aliases = {
        {QStringLiteral("zhhans"), QStringLiteral("zh-Hans")},
        {QStringLiteral("zhcn"), QStringLiteral("zh-Hans")},
        {QStringLiteral("zhsg"), QStringLiteral("zh-Hans")},
        {QStringLiteral("zh"), QStringLiteral("zh-Hans")},
        {QStringLiteral("chs"), QStringLiteral("zh-Hans")},
        {QStringLiteral("simplifiedchinese"), QStringLiteral("zh-Hans")},
        {QStringLiteral("chinesesimplified"), QStringLiteral("zh-Hans")},
        {QStringLiteral("chinese"), QStringLiteral("zh-Hans")},
        {QStringLiteral("简体中文"), QStringLiteral("zh-Hans")},
        {QStringLiteral("中文"), QStringLiteral("zh-Hans")},
        {QStringLiteral("汉语"), QStringLiteral("zh-Hans")},

        {QStringLiteral("zhhant"), QStringLiteral("zh-Hant")},
        {QStringLiteral("zhtw"), QStringLiteral("zh-Hant")},
        {QStringLiteral("zhhk"), QStringLiteral("zh-Hant")},
        {QStringLiteral("cht"), QStringLiteral("zh-Hant")},
        {QStringLiteral("traditionalchinese"), QStringLiteral("zh-Hant")},
        {QStringLiteral("chinesetraditional"), QStringLiteral("zh-Hant")},
        {QStringLiteral("繁体中文"), QStringLiteral("zh-Hant")},
        {QStringLiteral("繁體中文"), QStringLiteral("zh-Hant")},

        {QStringLiteral("yue"), QStringLiteral("yue")},
        {QStringLiteral("cantonese"), QStringLiteral("yue")},
        {QStringLiteral("粤语"), QStringLiteral("yue")},

        {QStringLiteral("en"), QStringLiteral("en")},
        {QStringLiteral("enus"), QStringLiteral("en")},
        {QStringLiteral("engb"), QStringLiteral("en")},
        {QStringLiteral("eng"), QStringLiteral("en")},
        {QStringLiteral("english"), QStringLiteral("en")},
        {QStringLiteral("英语"), QStringLiteral("en")},
        {QStringLiteral("英文"), QStringLiteral("en")},

        {QStringLiteral("ja"), QStringLiteral("ja")},
        {QStringLiteral("jp"), QStringLiteral("ja")},
        {QStringLiteral("jpn"), QStringLiteral("ja")},
        {QStringLiteral("japanese"), QStringLiteral("ja")},
        {QStringLiteral("日语"), QStringLiteral("ja")},
        {QStringLiteral("日文"), QStringLiteral("ja")},
        {QStringLiteral("日本語"), QStringLiteral("ja")},

        {QStringLiteral("ko"), QStringLiteral("ko")},
        {QStringLiteral("kor"), QStringLiteral("ko")},
        {QStringLiteral("korean"), QStringLiteral("ko")},
        {QStringLiteral("韩语"), QStringLiteral("ko")},
        {QStringLiteral("韩文"), QStringLiteral("ko")},

        {QStringLiteral("fr"), QStringLiteral("fr")},
        {QStringLiteral("fra"), QStringLiteral("fr")},
        {QStringLiteral("french"), QStringLiteral("fr")},
        {QStringLiteral("法语"), QStringLiteral("fr")},

        {QStringLiteral("de"), QStringLiteral("de")},
        {QStringLiteral("deu"), QStringLiteral("de")},
        {QStringLiteral("ger"), QStringLiteral("de")},
        {QStringLiteral("german"), QStringLiteral("de")},
        {QStringLiteral("德语"), QStringLiteral("de")},

        {QStringLiteral("ru"), QStringLiteral("ru")},
        {QStringLiteral("rus"), QStringLiteral("ru")},
        {QStringLiteral("russian"), QStringLiteral("ru")},
        {QStringLiteral("俄语"), QStringLiteral("ru")},

        {QStringLiteral("es"), QStringLiteral("es")},
        {QStringLiteral("spa"), QStringLiteral("es")},
        {QStringLiteral("spanish"), QStringLiteral("es")},
        {QStringLiteral("西班牙语"), QStringLiteral("es")},

        {QStringLiteral("pt"), QStringLiteral("pt")},
        {QStringLiteral("por"), QStringLiteral("pt")},
        {QStringLiteral("portuguese"), QStringLiteral("pt")},
        {QStringLiteral("葡萄牙语"), QStringLiteral("pt")},

        {QStringLiteral("it"), QStringLiteral("it")},
        {QStringLiteral("ita"), QStringLiteral("it")},
        {QStringLiteral("italian"), QStringLiteral("it")},
        {QStringLiteral("意大利语"), QStringLiteral("it")},

        {QStringLiteral("ar"), QStringLiteral("ar")},
        {QStringLiteral("ara"), QStringLiteral("ar")},
        {QStringLiteral("arabic"), QStringLiteral("ar")},
        {QStringLiteral("阿拉伯语"), QStringLiteral("ar")},

        {QStringLiteral("hi"), QStringLiteral("hi")},
        {QStringLiteral("hin"), QStringLiteral("hi")},
        {QStringLiteral("hindi"), QStringLiteral("hi")},
        {QStringLiteral("印地语"), QStringLiteral("hi")},

        {QStringLiteral("th"), QStringLiteral("th")},
        {QStringLiteral("tha"), QStringLiteral("th")},
        {QStringLiteral("thai"), QStringLiteral("th")},
        {QStringLiteral("泰语"), QStringLiteral("th")},

        {QStringLiteral("vi"), QStringLiteral("vi")},
        {QStringLiteral("vie"), QStringLiteral("vi")},
        {QStringLiteral("vietnamese"), QStringLiteral("vi")},
        {QStringLiteral("越南语"), QStringLiteral("vi")},

        {QStringLiteral("id"), QStringLiteral("id")},
        {QStringLiteral("ind"), QStringLiteral("id")},
        {QStringLiteral("indonesian"), QStringLiteral("id")},
        {QStringLiteral("印尼语"), QStringLiteral("id")},

        {QStringLiteral("ms"), QStringLiteral("ms")},
        {QStringLiteral("may"), QStringLiteral("ms")},
        {QStringLiteral("malay"), QStringLiteral("ms")},
        {QStringLiteral("马来语"), QStringLiteral("ms")},

        {QStringLiteral("tr"), QStringLiteral("tr")},
        {QStringLiteral("tur"), QStringLiteral("tr")},
        {QStringLiteral("turkish"), QStringLiteral("tr")},
        {QStringLiteral("土耳其语"), QStringLiteral("tr")},

        {QStringLiteral("nl"), QStringLiteral("nl")},
        {QStringLiteral("dut"), QStringLiteral("nl")},
        {QStringLiteral("dutch"), QStringLiteral("nl")},
        {QStringLiteral("荷兰语"), QStringLiteral("nl")},

        {QStringLiteral("pl"), QStringLiteral("pl")},
        {QStringLiteral("pol"), QStringLiteral("pl")},
        {QStringLiteral("polish"), QStringLiteral("pl")},
        {QStringLiteral("波兰语"), QStringLiteral("pl")},
    };
    return aliases;
}

/**
 * 读取规范键到英文展示名的映射表。
 * @return 展示名映射表。
 */
const QHash<QString, QString> &languageDisplayNames()
{
    static const QHash<QString, QString> names = {
        {QStringLiteral("zh-Hans"), QStringLiteral("Simplified Chinese")},
        {QStringLiteral("zh-Hant"), QStringLiteral("Traditional Chinese")},
        {QStringLiteral("yue"), QStringLiteral("Cantonese")},
        {QStringLiteral("en"), QStringLiteral("English")},
        {QStringLiteral("ja"), QStringLiteral("Japanese")},
        {QStringLiteral("ko"), QStringLiteral("Korean")},
        {QStringLiteral("fr"), QStringLiteral("French")},
        {QStringLiteral("de"), QStringLiteral("German")},
        {QStringLiteral("ru"), QStringLiteral("Russian")},
        {QStringLiteral("es"), QStringLiteral("Spanish")},
        {QStringLiteral("pt"), QStringLiteral("Portuguese")},
        {QStringLiteral("it"), QStringLiteral("Italian")},
        {QStringLiteral("ar"), QStringLiteral("Arabic")},
        {QStringLiteral("hi"), QStringLiteral("Hindi")},
        {QStringLiteral("th"), QStringLiteral("Thai")},
        {QStringLiteral("vi"), QStringLiteral("Vietnamese")},
        {QStringLiteral("id"), QStringLiteral("Indonesian")},
        {QStringLiteral("ms"), QStringLiteral("Malay")},
        {QStringLiteral("tr"), QStringLiteral("Turkish")},
        {QStringLiteral("nl"), QStringLiteral("Dutch")},
        {QStringLiteral("pl"), QStringLiteral("Polish")},
    };
    return names;
}

}  // namespace

QString canonicalLanguageKey(const QString &language)
{
    const QString trimmed = language.trimmed();
    if (trimmed.isEmpty()) {
        // 1. 未指定目标语言时沿用应用默认的简体中文
        return QStringLiteral("zh-Hans");
    }
    return languageAliases().value(normalizedInput(trimmed));
}

QString canonicalLanguageDisplayName(const QString &canonicalKey)
{
    return languageDisplayNames().value(canonicalKey, canonicalKey);
}

}  // namespace markshot::translate_common
