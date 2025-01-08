#include "infotable.h"
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextTableFormat>

InfoTable::InfoTable(QTextCursor &cursor, int rows, int cols)
    : m_columnAlignments(cols, Qt::AlignLeft) {
  QTextTableFormat format;
  format.setCellPadding(5.0);
  format.setBorderStyle(QTextFrameFormat::BorderStyle_None);
  format.setWidth(QTextLength(QTextLength::PercentageLength, 100));
  format.setCellSpacing(-1.0);
  format.setBorder(1.0);
  m_table = cursor.insertTable(rows, cols, format);
}

void InfoTable::insertTableHeader(const QStringList &tableHeader) {
  for (int col = 0; col < tableHeader.size(); ++col) {
    QTextTableCell cell = m_table->cellAt(0, col);
    QTextCursor cellCursor = cell.firstCursorPosition();

    // Set bold font for header
    QTextCharFormat charFormat = cellCursor.charFormat();
    charFormat.setFontWeight(QFont::Bold);
    cellCursor.setCharFormat(charFormat);

    cellCursor.insertText(tableHeader[col]);

    setCellAlignment(cell, m_columnAlignments[col], true);
  }
}

void InfoTable::insertColorBlock(int row, int column, const QColor &color,
                                 const QString &text) {
  if (!color.isValid()) {
    return;
  }

  QTextTableCell cell = m_table->cellAt(row, column);
  QTextCharFormat format = cell.format();
  format.setBackground(color);
  cell.setFormat(format);

  QTextCursor cursor = getCellCursor(row, column);
  cursor.insertText(text);
}

void InfoTable::setColumnAlignment(int column, Qt::Alignment alignment) {
  if (column >= 0 && column < m_columnAlignments.size()) {
    m_columnAlignments[column] = alignment;

    // Update all cells in the column
    for (int row = 0; row < m_table->rows(); ++row) {
      QTextTableCell cell = m_table->cellAt(row, column);
      setCellAlignment(cell, alignment, row == 0);
    }
  }
}

void InfoTable::insertCellValue(int row, int column, const QString &valueString,
                                Qt::Alignment alignment) {
  QTextCursor cursor = getCellCursor(row, column);
  QTextBlockFormat blockFormat = cursor.blockFormat();

  // Preserve vertical alignment
  Qt::Alignment vertAlign = blockFormat.alignment() & Qt::AlignVertical_Mask;
  Qt::Alignment combAlign = alignment | vertAlign;

  blockFormat.setAlignment(combAlign);

  // Update the column alignment if it's different
  if (combAlign != m_columnAlignments[column]) {
    setColumnAlignment(column, combAlign);
  }
  cursor.setBlockFormat(blockFormat);
  cursor.insertText(valueString);
}

void InfoTable::setCellAlignment(const QTextTableCell &cell,
                                 Qt::Alignment alignment, bool isHeader) {
  QTextCursor cellCursor = cell.firstCursorPosition();
  QTextBlockFormat blockFormat = cellCursor.blockFormat();
  blockFormat.setAlignment(alignment);
  cellCursor.setBlockFormat(blockFormat);

  if (isHeader) {
    QTextCharFormat charFormat = cellCursor.charFormat();
    charFormat.setFontWeight(QFont::Bold);
    cellCursor.setCharFormat(charFormat);
  }
}

QTextCursor InfoTable::getCellCursor(int row, int column) const {
  return m_table->cellAt(row, column).firstCursorPosition();
}
