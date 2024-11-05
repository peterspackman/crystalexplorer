#pragma once
#include <QGroupBox>

class CollapsibleGroupBox : public QGroupBox {
  Q_OBJECT

public:
  explicit CollapsibleGroupBox(QWidget *parent = nullptr);
  explicit CollapsibleGroupBox(const QString &title, QWidget *parent = nullptr);
  void setCollapsed(bool collapse);
  bool isCollapsed() const { return m_collapsed; }

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  bool event(QEvent *e) override;

private:
  void init();
  void toggleCollapsed();
  void hideContent(bool hide);
  QSize minimumSizeHint() const override;
  QSize sizeHint() const override;

  bool m_collapsed;
  int m_savedHeight;
};
