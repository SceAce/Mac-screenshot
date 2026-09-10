#pragma once

#include <QString>
#include <QStringList>

namespace markshot::translate_youdao {

/**
 * 计算单段文本的签名 input。
 *
 * 有道 v3 签名要求文本长度超过 20 时只取首尾各 10 个字符并在中间嵌入长度，
 * 长度按字符计数而非字节计数。
 *
 * @param query 待翻译文本。
 * @return 参与签名拼接的 input 串。
 */
QString youdaoSignInput(const QString &query);

/**
 * 计算多段文本的签名 input。
 *
 * 批量请求把全部段落无分隔符直接连接后再套用截断规则，与官方 demo 保持一致。
 *
 * @param queries 待翻译文本列表，顺序需与表单中 q 字段顺序一致。
 * @return 参与签名拼接的 input 串。
 */
QString youdaoSignInput(const QStringList &queries);

/**
 * 计算有道 v3 签名。
 *
 * 拼接顺序为 appKey + input + salt + curtime + appSecret，中间无分隔符。
 *
 * @param appKey 应用 ID。
 * @param signInput 由 youdaoSignInput 计算出的 input 串。
 * @param salt 本次请求的随机串。
 * @param curtime UTC 秒级时间戳字符串。
 * @param appSecret 应用密钥。
 * @return 64 位小写十六进制的 SHA256 摘要。
 */
QString youdaoSignature(const QString &appKey,
                        const QString &signInput,
                        const QString &salt,
                        const QString &curtime,
                        const QString &appSecret);

}  // namespace markshot::translate_youdao
