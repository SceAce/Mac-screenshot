#include "tencent_tc3_signer.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QMessageAuthenticationCode>
#include <QTimeZone>

namespace markshot::translate_tencent {
namespace {

/** 签名算法名称，同时出现在待签名串首行与 Authorization 头前缀。 */
const QByteArray kAlgorithm = QByteArrayLiteral("TC3-HMAC-SHA256");

/** 参与签名的请求头集合，与腾讯云官方 SDK 的最小集保持一致。 */
const QByteArray kSignedHeaders = QByteArrayLiteral("content-type;host");

/** 凭证范围结尾的固定终止串。 */
const QByteArray kRequestSuffix = QByteArrayLiteral("tc3_request");

/**
 * 计算 SHA256 摘要的小写十六进制表示。
 * @param data 待摘要数据。
 * @return 小写十六进制摘要。
 */
QByteArray sha256Hex(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex();
}

/**
 * 计算 HMAC-SHA256。
 * @param key 二进制密钥。
 * @param message 待签名消息。
 * @return 二进制签名结果。
 */
QByteArray hmacSha256(const QByteArray &key, const QByteArray &message)
{
    return QMessageAuthenticationCode::hash(message, key, QCryptographicHash::Sha256);
}

}  // namespace

QString tc3UtcDate(qint64 timestamp)
{
    // 1. 用 QTimeZone::utc() 而非 QTimeZone::UTC，后者需要 Qt 6.5 而项目底线是 Qt 6.2
    return QDateTime::fromSecsSinceEpoch(timestamp, QTimeZone::utc()).toString(QStringLiteral("yyyy-MM-dd"));
}

QString tc3CredentialScope(qint64 timestamp, const QString &service)
{
    return QStringLiteral("%1/%2/%3")
        .arg(tc3UtcDate(timestamp), service, QString::fromUtf8(kRequestSuffix));
}

QByteArray tc3CanonicalRequest(const Tc3SigningInput &input)
{
    // 1. CanonicalHeaders 的键值全部小写并去除首尾空白，按键名升序排列且每条自带换行
    QByteArray canonicalHeaders;
    canonicalHeaders.append(QByteArrayLiteral("content-type:"));
    canonicalHeaders.append(input.contentType.trimmed().toLower());
    canonicalHeaders.append('\n');
    canonicalHeaders.append(QByteArrayLiteral("host:"));
    canonicalHeaders.append(input.host.trimmed().toLower().toUtf8());
    canonicalHeaders.append('\n');

    // 2. 按固定顺序拼接六个部分，CanonicalQueryString 为空串因而形成一个空行
    QByteArray canonicalRequest;
    canonicalRequest.append(input.method);
    canonicalRequest.append('\n');
    canonicalRequest.append('/');
    canonicalRequest.append('\n');
    canonicalRequest.append('\n');

    // 3. CanonicalHeaders 结尾已有换行，再补一个换行后才是 SignedHeaders 所在行
    canonicalRequest.append(canonicalHeaders);
    canonicalRequest.append('\n');
    canonicalRequest.append(kSignedHeaders);
    canonicalRequest.append('\n');
    canonicalRequest.append(sha256Hex(input.payload));
    return canonicalRequest;
}

QByteArray tc3StringToSign(const Tc3SigningInput &input, const QByteArray &canonicalRequest)
{
    QByteArray stringToSign;
    stringToSign.append(kAlgorithm);
    stringToSign.append('\n');
    stringToSign.append(QByteArray::number(input.timestamp));
    stringToSign.append('\n');
    stringToSign.append(tc3CredentialScope(input.timestamp, input.service).toUtf8());
    stringToSign.append('\n');
    stringToSign.append(sha256Hex(canonicalRequest));
    return stringToSign;
}

Tc3DerivedKeys tc3DerivedKeys(const QString &secretKey, const QString &date, const QString &service)
{
    Tc3DerivedKeys keys;

    // 1. 三级派生的中间结果保持二进制，转成十六进制会得到完全不同的签名
    keys.secretDate = hmacSha256(QByteArrayLiteral("TC3") + secretKey.toUtf8(), date.toUtf8());
    keys.secretService = hmacSha256(keys.secretDate, service.toUtf8());
    keys.secretSigning = hmacSha256(keys.secretService, kRequestSuffix);
    return keys;
}

QByteArray tc3Signature(const Tc3SigningInput &input, const QByteArray &stringToSign)
{
    const Tc3DerivedKeys keys =
        tc3DerivedKeys(input.secretKey, tc3UtcDate(input.timestamp), input.service);
    return hmacSha256(keys.secretSigning, stringToSign).toHex();
}

QByteArray tc3AuthorizationHeader(const Tc3SigningInput &input)
{
    // 1. 依次算出规范请求串、待签名串与签名
    const QByteArray canonicalRequest = tc3CanonicalRequest(input);
    const QByteArray stringToSign = tc3StringToSign(input, canonicalRequest);
    const QByteArray signature = tc3Signature(input, stringToSign);

    // 2. Credential 之后是逗号加空格，分隔符缺一不可
    QByteArray header;
    header.append(kAlgorithm);
    header.append(QByteArrayLiteral(" Credential="));
    header.append(input.secretId.toUtf8());
    header.append('/');
    header.append(tc3CredentialScope(input.timestamp, input.service).toUtf8());
    header.append(QByteArrayLiteral(", SignedHeaders="));
    header.append(kSignedHeaders);
    header.append(QByteArrayLiteral(", Signature="));
    header.append(signature);
    return header;
}

}  // namespace markshot::translate_tencent
