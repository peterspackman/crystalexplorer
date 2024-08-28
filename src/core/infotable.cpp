#include "infotable.h"
#include <QTextTableFormat>
#include <QTextCharFormat>
#include <QTextBlockFormat>

InfoTable::InfoTable(QTextCursor& cursor, int rows, int cols)
{
    QTextTableFormat format;
    format.setCellPadding(5.0);
    format.setBorderStyle(QTextFrameFormat::BorderStyle_None);
    format.setCellSpacing(-1.0);
    format.setBorder(1.0);

    m_table = cursor.insertTable(rows, cols, format);
}

InfoTable::~InfoTable()
{
    // The QTextTable object is owned by the QTextDocument,
    // so we don't need to delete it here.
}

void InfoTable::insertTableHeader(const QStringList& tableHeader)
{
    const int row = 0;
    QTextCharFormat format;
    format.setFontWeight(QFont::Bold);

    for (int column = 0; column < tableHeader.size() && column < m_table->columns(); ++column) {
        QTextCursor cursor = getCellCursor(row, column);
        cursor.setCharFormat(format);
        cursor.insertText(tableHeader[column]);
    }
}

void InfoTable::insertColorBlock(int row, int column, const QColor& color)
{
    if (!color.isValid()) {
        return;
    }

    QTextTableCell cell = m_table->cellAt(row, column);
    QTextCharFormat format = cell.format();
    format.setBackground(color);
    cell.setFormat(format);

    QTextCursor cursor = getCellCursor(row, column);
    cursor.insertText("     ");
}

void InfoTable::insertCellValue(int row, int column, const QString& valueString, Qt::Alignment alignment)
{
    QTextCursor cursor = getCellCursor(row, column);
    QTextBlockFormat blockFormat = cursor.blockFormat();
    
    // Preserve vertical alignment
    Qt::Alignment vertAlign = blockFormat.alignment() & Qt::AlignVertical_Mask;
    Qt::Alignment combAlign = alignment | vertAlign;
    
    blockFormat.setAlignment(combAlign);
    cursor.setBlockFormat(blockFormat);
    cursor.insertText(valueString);
}

QTextCursor InfoTable::getCellCursor(int row, int column) const
{
    return m_table->cellAt(row, column).firstCursorPosition();
}
