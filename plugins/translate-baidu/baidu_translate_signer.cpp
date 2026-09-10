#include "baidu_translate_signer.h"

#include <QCryptographicHash>
#include <QRandomGenerator>

namespace markshot::translate_baidu {

QString baiduSignatureSource(const QString &appId,
                             const QString &query,
                             const QString &salt,
                             const QString &appKey)
{
    // 1. 顺序固定为 appid + q + salt + 密钥，任何分隔符或顺序调整都会导致签名不匹配
    return appId + query + salt + appKey;
}

QString baiduSignature(const QString &appId,
                       const QString &query,
                       const QString &salt,
                       const QString &appKey)
{
    // 1. 签名原文按 UTF-8 编码后取 MD5，服务端以同样方式还原并比对
    const QByteArray digest =
        QCryptographicHash::hash(baiduSignatureSource(appId, query, salt, appKey).toUtf8(),
                                 QCryptographicHash::Md5);

    // 2. toHex 输出小写十六进制，长度固定 32 位，符合平台要求
    return QString::fromLatin1(digest.toHex());
}

QString generateBaiduSalt()
{
    // 1. 每次请求取新的随机数，保证同一段文本在多次请求中的签名不同
    return QString::number(QRandomGenerator::global()->generate());
}

}  // namespace markshot::translate_baidu
