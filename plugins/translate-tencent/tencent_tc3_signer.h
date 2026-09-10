#pragma once

#include <QByteArray>
#include <QString>

namespace markshot::translate_tencent {

/**
 * TC3-HMAC-SHA256 签名输入。
 *
 * payload 必须与实际发送的请求体逐字节相同，contentType 也必须与请求头一致，
 * 否则服务端重算的签名不会匹配。
 */
struct Tc3SigningInput {
    QByteArray method = QByteArrayLiteral("POST");
    QString host;
    QByteArray contentType = QByteArrayLiteral("application/json; charset=utf-8");
    QByteArray payload;
    QString service = QStringLiteral("tmt");
    QString secretId;
    QString secretKey;
    qint64 timestamp = 0;
};

/**
 * TC3 派生密钥的三级中间量。
 *
 * 三个字段都是二进制 HMAC 输出，只有最终签名才转成十六进制。
 */
struct Tc3DerivedKeys {
    QByteArray secretDate;
    QByteArray secretService;
    QByteArray secretSigning;
};

/**
 * 把秒级时间戳换算成 UTC 日期。
 *
 * 凭证范围里的日期必须按 UTC 计算，用本地时区会在跨日时段签名失败。
 *
 * @param timestamp 秒级时间戳。
 * @return yyyy-MM-dd 形式的 UTC 日期。
 */
QString tc3UtcDate(qint64 timestamp);

/**
 * 构造凭证范围。
 * @param timestamp 秒级时间戳。
 * @param service 服务名，例如 tmt。
 * @return <UTC 日期>/<service>/tc3_request。
 */
QString tc3CredentialScope(qint64 timestamp, const QString &service);

/**
 * 构造规范请求串。
 *
 * 签名头集合固定为 content-type;host，与腾讯云官方 SDK 的最小集一致。
 *
 * @param input 签名输入。
 * @return 规范请求串，CanonicalQueryString 与 CanonicalHeaders 之后各有一个空行。
 */
QByteArray tc3CanonicalRequest(const Tc3SigningInput &input);

/**
 * 构造待签名串。
 * @param input 签名输入。
 * @param canonicalRequest 规范请求串。
 * @return 待签名串。
 */
QByteArray tc3StringToSign(const Tc3SigningInput &input, const QByteArray &canonicalRequest);

/**
 * 逐级派生签名密钥。
 * @param secretKey 账号密钥。
 * @param date UTC 日期，形如 2019-02-25。
 * @param service 服务名。
 * @return 三级派生密钥，全部为二进制内容。
 */
Tc3DerivedKeys tc3DerivedKeys(const QString &secretKey, const QString &date, const QString &service);

/**
 * 计算请求签名。
 * @param input 签名输入。
 * @param stringToSign 待签名串。
 * @return 小写十六进制签名。
 */
QByteArray tc3Signature(const Tc3SigningInput &input, const QByteArray &stringToSign);

/**
 * 构造 Authorization 请求头。
 * @param input 签名输入。
 * @return 完整的 Authorization 头取值。
 */
QByteArray tc3AuthorizationHeader(const Tc3SigningInput &input);

}  // namespace markshot::translate_tencent
