#include "collapsiblegroupbox.h"
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionGroupBox>
#include <QVBoxLayout>

CollapsibleGroupBox::CollapsibleGroupBox(QWidget *parent)
    : QGroupBox(parent), m_collapsed(false), m_savedHeight(0) {
  init();
}

CollapsibleGroupBox::CollapsibleGroupBox(const QString &title, QWidget *parent)
    : QGroupBox(title, parent), m_collapsed(false), m_savedHeight(0) {
  init();
}

void CollapsibleGroupBox::init() {
  setFlat(true);
}

void CollapsibleGroupBox::hideContent(bool hide) {
  QLayout *lay = layout();
  if (!lay)
    return;

  for (int i = 0; i < lay->count(); ++i) {
    QLayoutItem *item = lay->itemAt(i);
    if (item->widget()) {
      item->widget()->setVisible(!hide);
    }
    // Also handle nested layouts
    if (item->layout()) {
      QLayout *childLayout = item->layout();
      for (int j = 0; j < childLayout->count(); ++j) {
        QWidget *w = childLayout->itemAt(j)->widget();
        if (w) {
          w->setVisible(!hide);
        }
      }
    }
  }

  // Ensure layout spacing is also collapsed
  if (lay) {
    lay->setSpacing(
        hide ? 0 : style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing));
    lay->setContentsMargins(
        lay->contentsMargins().left(),
        hide ? 0 : style()->pixelMetric(QStyle::PM_LayoutTopMargin),
        lay->contentsMargins().right(),
        hide ? 0 : style()->pixelMetric(QStyle::PM_LayoutBottomMargin));
  }
}

void CollapsibleGroupBox::setCollapsed(bool collapse) {
  if (m_collapsed != collapse) {
    toggleCollapsed();
  }
}

void CollapsibleGroupBox::toggleCollapsed() {
  m_collapsed = !m_collapsed;

  if (m_collapsed) {
    m_savedHeight = height();
    hideContent(true);
    setMaximumHeight(minimumSizeHint().height());
    setMinimumHeight(minimumSizeHint().height());
  } else {
    setMinimumHeight(0);
    setMaximumHeight(QWIDGETSIZE_MAX);
    hideContent(false);
  }

  updateGeometry();

  if (parentWidget() && parentWidget()->layout()) {
    parentWidget()->layout()->invalidate();
  }
}

bool CollapsibleGroupBox::event(QEvent *e) {
  if (e->type() == QEvent::LayoutRequest && !m_collapsed) {
    setMaximumHeight(QWIDGETSIZE_MAX);
  }
  return QGroupBox::event(e);
}

QSize CollapsibleGroupBox::minimumSizeHint() const {
  if (m_collapsed) {
    QStyleOptionGroupBox option;
    option.initFrom(this);
    option.text = title();
    QRect titleRect = style()->subControlRect(QStyle::CC_GroupBox, &option,
                                              QStyle::SC_GroupBoxLabel, this);
    int titleHeight =
        titleRect.height() + style()->pixelMetric(QStyle::PM_LayoutTopMargin);
    return QSize(QGroupBox::minimumSizeHint().width(), titleHeight);
  }
  return QGroupBox::minimumSizeHint();
}

QSize CollapsibleGroupBox::sizeHint() const {
  if (m_collapsed) {
    return minimumSizeHint();
  }
  return QGroupBox::sizeHint();
}

void CollapsibleGroupBox::mousePressEvent(QMouseEvent *event) {
  QStyleOptionGroupBox option;
  option.initFrom(this);
  option.text = title();
  QRect titleRect = style()->subControlRect(QStyle::CC_GroupBox, &option,
                                            QStyle::SC_GroupBoxLabel, this);

  // Calculate indicator rect (making it a bit wider than the text for easier
  // clicking)
  QString indicator = m_collapsed ? "[+]" : "[-]";
  QRect indicatorRect(titleRect.topRight() + QPoint(4, 0),
                      QSize(fontMetrics().horizontalAdvance(indicator) + 8,
                            titleRect.height()));

  // Check if click is in title area or indicator area
  if (titleRect.contains(event->pos()) ||
      indicatorRect.contains(event->pos())) {
    toggleCollapsed();
  }
  QGroupBox::mousePressEvent(event);
}

void CollapsibleGroupBox::paintEvent(QPaintEvent *event) {
  QGroupBox::paintEvent(event);

  QPainter painter(this);
  QStyleOptionGroupBox option;
  option.initFrom(this);
  option.text = title();
  QRect titleRect = style()->subControlRect(QStyle::CC_GroupBox, &option,
                                            QStyle::SC_GroupBoxLabel, this);

  // Draw [+] or [-] after the title text
  QString indicator = m_collapsed ? "[+]" : "[-]";
  painter.setPen(palette().windowText().color());

  // Calculate exact vertical position to align with text baseline
  QFontMetrics fm = fontMetrics();
  int textHeight = fm.height();
  int baseline =
      titleRect.top() + (titleRect.height() - textHeight) / 2 + fm.ascent();

  painter.drawText(QPoint(titleRect.right() + 4, baseline), indicator);
}
