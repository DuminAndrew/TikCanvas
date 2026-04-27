#pragma once
#include <QImage>
#include <QVector>
#include "../widgets/MapCanvas.h"

class Vectorizer
{
public:
    static QVector<DrawStroke> extractLines(const QImage &src,
                                            int threshold = 150,
                                            int minLength = 25,
                                            int sampleEvery = 4);
};
