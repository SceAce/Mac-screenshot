#include "shot_window_line_constraint.h"

#include <QtGlobal>

#include <cmath>

namespace {

constexpr qreal kPi = 3.14159265358979323846;
constexpr qreal kQuarterPi = kPi / 4.0;

}  // namespace

namespace markshot::shot {

QPointF constrainedLineEnd(QPointF start, QPointF end)
{
    const QPointF delta = end - start;
    if (qFuzzyIsNull(delta.x()) && qFuzzyIsNull(delta.y())) {
        return end;
    }

    const qreal angle = std::atan2(delta.y(), delta.x());
    const qreal snappedAngle = std::round(angle / kQuarterPi) * kQuarterPi;
    const QPointF direction(std::cos(snappedAngle), std::sin(snappedAngle));
    const qreal projectedLength = QPointF::dotProduct(delta, direction);
    return start + direction * projectedLength;
}

}  // namespace markshot::shot
