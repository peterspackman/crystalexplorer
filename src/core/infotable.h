#pragma once
#include <QTextTable>
#include <QTextCursor>
#include <QStringList>
#include <QColor>

class InfoTable {
public:
    InfoTable(QTextCursor& cursor, int rows, int cols);
    ~InfoTable();

    void insertTableHeader(const QStringList& tableHeader);
    void insertColorBlock(int row, int column, const QColor& color);
    void insertCellValue(int row, int column, const QString& valueString, Qt::Alignment alignment = Qt::AlignLeft);

    QTextTable* table() const { return m_table; }

private:
    QTextCursor getCellCursor(int row, int column) const;
    QTextTable* m_table;
};
