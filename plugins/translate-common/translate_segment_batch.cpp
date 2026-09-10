#include "translate_segment_batch.h"

namespace markshot::translate_common {

QString flattenSegmentText(const QString &text)
{
    QString flattened = text;
    flattened.replace(QLatin1Char('\r'), QLatin1Char(' '));
    flattened.replace(QLatin1Char('\n'), QLatin1Char(' '));
    flattened.replace(QLatin1Char('\t'), QLatin1Char(' '));
    return flattened.trimmed();
}

QVector<QVector<markshot::plugin::TranslateSegment>> chunkTranslateSegments(
    const QVector<markshot::plugin::TranslateSegment> &segments,
    int maxChars)
{
    QVector<QVector<markshot::plugin::TranslateSegment>> batches;
    if (segments.isEmpty()) {
        return batches;
    }
    if (maxChars <= 0) {
        batches.append(segments);
        return batches;
    }

    QVector<markshot::plugin::TranslateSegment> current;
    int currentChars = 0;
    for (const markshot::plugin::TranslateSegment &segment : segments) {
        const int segmentChars = segment.text.size();

        // 1. 当前批次装不下就先结算，单段超预算时独立成批
        if (!current.isEmpty() && currentChars + segmentChars > maxChars) {
            batches.append(current);
            current.clear();
            currentChars = 0;
        }
        current.append(segment);
        currentChars += segmentChars;
    }
    if (!current.isEmpty()) {
        batches.append(current);
    }
    return batches;
}

}  // namespace markshot::translate_common
