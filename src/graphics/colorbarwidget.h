#pragma once
#include <QWidget>
#include "colormap.h"

class ColorBarWidget : public QWidget {
    Q_OBJECT

public:
    explicit ColorBarWidget(QWidget *parent = nullptr);

    void setColorMap(const QString &colorMapName, double minValue, double maxValue);
    void setLabel(const QString &label);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString _colorMapName{"BlueWhiteRed"};
    double _minValue{-1.0};
    double _maxValue{1.0};
    QString _label;

    // Visual parameters
    static constexpr int BAR_WIDTH = 20;
    static constexpr int BAR_HEIGHT = 200;
    static constexpr int MARGIN = 10;
    static constexpr int LABEL_SPACING = 5;
};
