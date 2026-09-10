#pragma once

#include <QString>

namespace markshot::translate_baidu {

/**
 * 拼接百度翻译开放平台的签名原文。
 *
 * 平台要求把 appid、待翻译文本、salt 与密钥首尾直接相连，中间不加任何分隔符。
 * 这里的待翻译文本必须是未经 URL 编码的原文，编码只允许发生在构造请求体时，
 * 否则服务端会返回 54001 签名错误。
 *
 * @param appId 开放平台 APPID。
 * @param query 待翻译文本，多段时已用换行连接。
 * @param salt 本次请求使用的随机数。
 * @param appKey 开放平台密钥。
 * @return 参与 MD5 计算的签名原文。
 */
QString baiduSignatureSource(const QString &appId,
                             const QString &query,
                             const QString &salt,
                             const QString &appKey);

/**
 * 计算百度翻译请求签名。
 *
 * @param appId 开放平台 APPID。
 * @param query 待翻译文本，多段时已用换行连接。
 * @param salt 本次请求使用的随机数。
 * @param appKey 开放平台密钥。
 * @return 签名原文按 UTF-8 编码后的 32 位小写十六进制 MD5 值。
 */
QString baiduSignature(const QString &appId,
                       const QString &query,
                       const QString &salt,
                       const QString &appKey);

/**
 * 生成本次请求使用的随机数。
 * @return 十进制数字组成的随机字符串。
 */
QString generateBaiduSalt();

}  // namespace markshot::translate_baidu
