#include "youdao_translate_signer.h"

#include <QCryptographicHash>

namespace markshot::translate_youdao {
namespace {

// 签名 input 的截断阈值与首尾保留长度，取值来自有道 v3 签名规范
constexpr int kSignInputFullLength = 20;
constexpr int kSignInputEdgeLength = 10;

}  // namespace

QString youdaoSignInput(const QString &query)
{
    // 1. 长度以 QString 的字符数计量，与官方 demo 的 UTF-16 计数语义一致
    const int length = query.length();
    if (length <= kSignInputFullLength) {
        return query;
    }

    // 2. 超长文本取首 10 个字符、长度十进制、尾 10 个字符拼接
    return query.left(kSignInputEdgeLength) + QString::number(length) + query.right(kSignInputEdgeLength);
}

QString youdaoSignInput(const QStringList &queries)
{
    // 1. 多段文本先无分隔符直接连接，再按单段规则截断
    QString joined;
    int reserved = 0;
    for (const QString &query : queries) {
        reserved += query.length();
    }
    joined.reserve(reserved);
    for (const QString &query : queries) {
        joined.append(query);
    }
    return youdaoSignInput(joined);
}

QString youdaoSignature(const QString &appKey,
                        const QString &signInput,
                        const QString &salt,
                        const QString &curtime,
                        const QString &appSecret)
{
    // 1. 按固定顺序拼接待签名串，任何多余分隔符都会导致 202 签名校验失败
    const QString source = appKey + signInput + salt + curtime + appSecret;

    // 2. 以 UTF-8 编码求 SHA256，toHex 输出即为小写十六进制
    const QByteArray digest = QCryptographicHash::hash(source.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromLatin1(digest.toHex());
}

}  // namespace markshot::translate_youdao
