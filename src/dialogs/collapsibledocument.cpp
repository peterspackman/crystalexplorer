#include "collapsibledocument.h"
#include <QPainter>
#include <QTextBlock>

CollapsibleSectionHeader::CollapsibleSectionHeader(const QString &title,
                                                   QWidget *parent)
    : QWidget(parent), m_title(title), m_collapsed(false) {
  setFixedHeight(30); // Adjust as needed
}

void CollapsibleSectionHeader::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.drawText(rect(), Qt::AlignLeft | Qt::AlignVCenter, m_title);
  painter.drawText(rect().adjusted(width() - 20, 0, 0, 0), Qt::AlignCenter,
                   m_collapsed ? "▶" : "▼");
}

void CollapsibleSectionHeader::mousePressEvent(QMouseEvent *event) {
  emit toggleCollapsed();
}

void CollapsibleSectionHeader::setCollapsed(bool collapsed) {
  m_collapsed = collapsed;
  update();
}

CollapsibleDocumentWidget::CollapsibleDocumentWidget(QWidget *parent)
    : QWidget(parent) {
  m_layout = new QVBoxLayout(this);
  m_textEdit = new QTextEdit(this);
  m_layout->addWidget(m_textEdit);
}

void CollapsibleDocumentWidget::insertSection(const QString &title,
                                              const QString &content) {
  CollapsibleSectionHeader *header = new CollapsibleSectionHeader(title, this);
  connect(header, &CollapsibleSectionHeader::toggleCollapsed, this,
          &CollapsibleDocumentWidget::toggleSection);
  m_layout->insertWidget(m_layout->count() - 1, header);
  m_headers.append(header);

  QTextCursor cursor(m_textEdit->document());
  cursor.movePosition(QTextCursor::End);

  int startPosition = cursor.position();
  cursor.insertText(content);
  int endPosition = cursor.position();

  m_sectionRanges[header] = qMakePair(startPosition, endPosition);
}

void CollapsibleDocumentWidget::toggleSection() {
  CollapsibleSectionHeader *header =
      qobject_cast<CollapsibleSectionHeader *>(sender());
  if (!header || !m_sectionRanges.contains(header))
    return;

  header->setCollapsed(!header->isCollapsed());

  QPair<int, int> range = m_sectionRanges[header];
  QTextDocument *doc = m_textEdit->document();
  QTextBlock block = doc->findBlock(range.first);
  QTextBlock end = doc->findBlock(range.second).next();

  while (block != end) {
    block.setVisible(!header->isCollapsed());
    block = block.next();
  }

  // Force update of the viewport
  m_textEdit->viewport()->update();

  // Adjust document height
  updateSectionRanges();
}

void CollapsibleDocumentWidget::updateSectionRanges() {
  QTextDocument *doc = m_textEdit->document();
  int currentPos = 0;

  for (CollapsibleSectionHeader *header : m_headers) {
    if (m_sectionRanges.contains(header)) {
      QPair<int, int> &range = m_sectionRanges[header];
      range.first = currentPos;

      if (!header->isCollapsed()) {
        QTextBlock block = doc->findBlock(range.first);
        QTextBlock end = doc->findBlock(range.second).next();

        while (block != end) {
          if (block.isVisible()) {
            currentPos += block.length();
          }
          block = block.next();
        }
      }

      range.second = currentPos;
    }
  }

  // Update document size
  doc->adjustSize();
}
