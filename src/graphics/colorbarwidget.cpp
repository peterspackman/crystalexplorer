#include "colorbarwidget.h"
#include "settings.h"
#include <QPainter>
#include <QLinearGradient>
#include <QFontMetrics>

ColorBarWidget::ColorBarWidget(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setFixedSize(BAR_WIDTH + 2 * MARGIN + 30, BAR_HEIGHT + 2 * MARGIN + 40);
}

void ColorBarWidget::setColorMap(const QString &colorMapName, double minValue, double maxValue) {
    _colorMapName = colorMapName;
    _minValue = minValue;
    _maxValue = maxValue;
    update();
}

void ColorBarWidget::setLabel(const QString &label) {
    _label = label;
    update();
}

void ColorBarWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Create colormap
    ColorMap cmap(_colorMapName, _minValue, _maxValue);

    // Draw gradient bar
    int barX = MARGIN + 15;  // Leave space for vertical title
    int barY = MARGIN + 15;  // Leave space for top label

    // Sample colormap to create gradient stops
    QLinearGradient gradient(barX, barY + BAR_HEIGHT, barX, barY);
    constexpr int NUM_SAMPLES = 20;
    for (int i = 0; i <= NUM_SAMPLES; ++i) {
        double t = static_cast<double>(i) / NUM_SAMPLES;
        double value = _minValue + t * (_maxValue - _minValue);
        QColor color = cmap(value);
        gradient.setColorAt(t, color);
    }

    // Draw bar with border
    painter.setPen(Qt::black);
    painter.setBrush(gradient);
    painter.drawRect(barX, barY, BAR_WIDTH, BAR_HEIGHT);

    // Draw labels using the same text color as other labels
    QColor textColor = QColor(settings::readSetting(settings::keys::TEXT_COLOR).toString());
    painter.setPen(textColor);
    painter.setBrush(Qt::NoBrush);

    QFont font = painter.font();
    font.setPointSize(10);
    painter.setFont(font);

    // Max value above bar
    QString maxText = QString::number(_maxValue, 'f', 2);
    QFontMetrics fm(font);
    int maxTextWidth = fm.horizontalAdvance(maxText);
    painter.drawText(barX + (BAR_WIDTH - maxTextWidth) / 2, barY - 3, maxText);

    // Min value below bar
    QString minText = QString::number(_minValue, 'f', 2);
    int minTextWidth = fm.horizontalAdvance(minText);
    painter.drawText(barX + (BAR_WIDTH - minTextWidth) / 2, barY + BAR_HEIGHT + 12, minText);

    // Optional vertical label/title on left side
    if (!_label.isEmpty()) {
        painter.save();
        QFont titleFont = font;
        titleFont.setPointSize(12);
        painter.setFont(titleFont);
        painter.translate(MARGIN, barY + BAR_HEIGHT / 2);
        painter.rotate(-90);
        QRect titleRect(-BAR_HEIGHT / 2, -10, BAR_HEIGHT, 20);
        painter.drawText(titleRect, Qt::AlignHCenter | Qt::AlignVCenter, _label);
        painter.restore();
    }
}
