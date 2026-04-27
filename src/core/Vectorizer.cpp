#include "Vectorizer.h"
#include <QImage>
#include <QPainter>
#include <QtMath>

QVector<DrawStroke> Vectorizer::extractLines(const QImage &src,
                                             int threshold,
                                             int minLength,
                                             int sampleEvery)
{
    Q_UNUSED(sampleEvery);
    QVector<DrawStroke> out;
    if (src.isNull()) return out;

    QImage g = src.convertToFormat(QImage::Format_Grayscale8);
    const int W = g.width();
    const int H = g.height();

    // ---- Horizontal segments: every row, dense ----
    for (int y = 0; y < H; y += 2) {
        const uchar *row = g.scanLine(y);
        int runStart = -1;
        for (int x = 0; x < W; ++x) {
            const bool dark = row[x] < threshold;
            if (dark && runStart < 0) runStart = x;
            else if ((!dark || x == W - 1) && runStart >= 0) {
                const int len = x - runStart;
                if (len >= minLength) {
                    DrawStroke s;
                    s.kind  = "pen";
                    s.color = "#FFFFFF";
                    s.width = 2;
                    s.points = { QPointF(runStart, y), QPointF(x, y) };
                    out.append(s);
                }
                runStart = -1;
            }
        }
    }

    // ---- Vertical segments ----
    for (int x = 0; x < W; x += 2) {
        int runStart = -1;
        for (int y = 0; y < H; ++y) {
            const bool dark = g.scanLine(y)[x] < threshold;
            if (dark && runStart < 0) runStart = y;
            else if ((!dark || y == H - 1) && runStart >= 0) {
                const int len = y - runStart;
                if (len >= minLength) {
                    DrawStroke s;
                    s.kind  = "pen";
                    s.color = "#FFFFFF";
                    s.width = 2;
                    s.points = { QPointF(x, runStart), QPointF(x, y) };
                    out.append(s);
                }
                runStart = -1;
            }
        }
    }

    if (out.size() > 8000) out.resize(8000);
    return out;
}
