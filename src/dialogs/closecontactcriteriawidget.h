#pragma once
#include "globals.h"
#include "close_contact_criteria.h"
#include "colormap.h"

#include <QWidget>
#include <QToolButton>
#include <QColor>
#include <QGridLayout>

class CloseContactCriteriaWidget : public QWidget {
  Q_OBJECT

public:
  CloseContactCriteriaWidget(QWidget *parent = nullptr);

  CloseContactCriteria getCriteria(int index);
  int count() const;
  void updateElements(const QList<QString> &elements);

signals:
  void closeContactsSettingsChanged(int, CloseContactCriteria);

public slots:
  void addRow();

private:
  double largestVdwRadiusForAllElements() const;
  void criteriaChanged(int);

  QList<QString> m_elements;
  void setButtonColor(QToolButton *, QColor);
  QColor getButtonColor(QToolButton *colorButton);
  void addHeader();
  QGridLayout *m_layout;
  double m_vdwMax{3.0};
  ColorMap m_colorMap;
};

