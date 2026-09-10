#include "code_scan_result.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QRectF>
#include <QString>

#include <iostream>

namespace {

/**
 * 记录测试失败信息。
 * @param condition 断言条件。
 * @param message 失败时输出的说明。
 * @return 断言成功时返回 0，否则返回 1。
 */
int expect(bool condition, const char *message)
{
    if (condition) {
        return 0;
    }
    std::cerr << message << '\n';
    return 1;
}

/**
 * 验证 helper 的对象输出解析。
 * @return 失败数量。
 */
int parsesObjectOutput()
{
    const QByteArray output = R"({
        "backend": "zxingcpp",
        "results": [
            {
                "format": "QRCode",
                "text": "https://example.test",
                "points": [[1, 2], [11, 2], [11, 12], [1, 12]]
            }
        ]
    })";

    const markshot::code_scan::ParsedOutput parsed =
        markshot::code_scan::parseOutput(output, QSize(20, 20));

    int failures = 0;
    failures += expect(parsed.validJson, "object output should be valid JSON");
    failures += expect(parsed.backend == QStringLiteral("zxingcpp"), "backend should be parsed");
    failures += expect(parsed.results.size() == 1, "object output should contain one result");
    if (!parsed.results.isEmpty()) {
        failures += expect(parsed.results.first().format == QStringLiteral("QRCode"), "format should be parsed");
        failures += expect(parsed.results.first().text == QStringLiteral("https://example.test"), "text should be parsed");
        failures += expect(parsed.results.first().imageRect == QRectF(1, 2, 10, 10), "points should form a rect");
    }
    return failures;
}

/**
 * 验证数组输出和边界裁剪。
 * @return 失败数量。
 */
int parsesArrayOutputAndBounds()
{
    const QByteArray output = R"([
        {"type": "Code128", "value": "123456", "box": [5, 6, 40, 12]},
        {"type": "QRCode", "value": "outside", "box": [100, 100, 10, 10]}
    ])";

    const markshot::code_scan::ParsedOutput parsed =
        markshot::code_scan::parseOutput(output, QSize(50, 50));

    int failures = 0;
    failures += expect(parsed.validJson, "array output should be valid JSON");
    failures += expect(parsed.results.size() == 2, "array output should contain two results");
    if (parsed.results.size() == 2) {
        failures += expect(parsed.results.at(0).imageRect == QRectF(5, 6, 40, 12), "first rect should be parsed");
        failures += expect(parsed.results.at(1).imageRect.isEmpty(), "outside rect should be clipped to empty");
    }
    return failures;
}

/**
 * 验证多结果展示文本。
 * @return 失败数量。
 */
int formatsMultipleResults()
{
    QVector<markshot::code_scan::Result> results;
    markshot::code_scan::Result first;
    first.format = QStringLiteral("QRCode");
    first.text = QStringLiteral("alpha");
    results.append(first);

    markshot::code_scan::Result second;
    second.format = QStringLiteral("EAN-13");
    second.text = QStringLiteral("9780000000000");
    results.append(second);

    return expect(markshot::code_scan::resultsText(results)
                      == QStringLiteral("1. QRCode\nalpha\n\n2. EAN-13\n9780000000000"),
                  "multiple result text should be formatted");
}

/**
 * 验证无效 JSON 解析。
 * @return 失败数量。
 */
int reportsInvalidJson()
{
    const markshot::code_scan::ParsedOutput parsed =
        markshot::code_scan::parseOutput(QByteArray("{"));

    int failures = 0;
    failures += expect(!parsed.validJson, "invalid JSON should be reported");
    failures += expect(parsed.results.isEmpty(), "invalid JSON should not produce results");
    return failures;
}

/**
 * 验证缺少扫码后端的错误识别。
 * @return 失败数量。
 */
int reportsMissingBackend()
{
    const QByteArray output = R"({"backend":"","results":[],"errors":["No module named zxingcpp"]})";
    return expect(markshot::code_scan::outputReportsMissingBackend(output, QByteArray()),
                  "missing backend should be detected");
}

}  // namespace

/**
 * 执行扫码结果解析测试。
 * @param argc 命令行参数数量。
 * @param argv 命令行参数。
 * @return 失败数量，0 表示通过。
 */
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);

    int failures = 0;
    failures += parsesObjectOutput();
    failures += parsesArrayOutputAndBounds();
    failures += formatsMultipleResults();
    failures += reportsInvalidJson();
    failures += reportsMissingBackend();
    return failures == 0 ? 0 : 1;
}
