#pragma once

#include "markshot/translate_provider_plugin.h"

#include <QString>
#include <QVector>

namespace markshot::translate_common {

/**
 * 把分段文本压平为单行。
 *
 * 百度与有道的批量协议用换行分隔多段文本，段内换行会让译文与 id 错位，因此发送
 * 前统一把换行与制表符折叠成空格。
 *
 * @param text 原始分段文本。
 * @return 压平并去除首尾空白后的文本。
 */
QString flattenSegmentText(const QString &text);

/**
 * 按字符预算把分段拆分成多个批次。
 *
 * 单段本身超过预算时独立成批，交由调用方决定截断或直接发送，避免静默丢段。
 *
 * @param segments 待翻译分段。
 * @param maxChars 单批字符预算，非正数时不拆分。
 * @return 批次列表，保持输入顺序。
 */
QVector<QVector<markshot::plugin::TranslateSegment>> chunkTranslateSegments(
    const QVector<markshot::plugin::TranslateSegment> &segments,
    int maxChars);

}  // namespace markshot::translate_common
