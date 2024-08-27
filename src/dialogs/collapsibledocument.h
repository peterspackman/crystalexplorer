#pragma once
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

class CollapsibleSectionHeader : public QWidget {
  Q_OBJECT
public:
  CollapsibleSectionHeader(const QString &title, QWidget *parent = nullptr);
  void setCollapsed(bool collapsed);
  bool isCollapsed() const { return m_collapsed; }

signals:
  void toggleCollapsed();

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;

private:
  QString m_title;
  bool m_collapsed;
};

class CollapsibleDocumentWidget : public QWidget {
  Q_OBJECT
public:
  CollapsibleDocumentWidget(QWidget *parent = nullptr);
  void insertSection(const QString &title, const QString &content);
  QTextDocument *document() const { return m_textEdit->document(); }

private slots:
  void toggleSection();

private:
  QVBoxLayout *m_layout;
  QTextEdit *m_textEdit;
  QList<CollapsibleSectionHeader *> m_headers;
  QMap<CollapsibleSectionHeader *, QPair<int, int>> m_sectionRanges;

  void updateSectionRanges();
};
