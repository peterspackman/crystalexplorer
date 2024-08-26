#pragma once

#include <QObject>
#include <QStyledItemDelegate>

class ColorDelegate : public QStyledItemDelegate {
  Q_OBJECT

public:
  explicit ColorDelegate(QObject *parent = nullptr);

  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;

  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
                        const QModelIndex &) const override;

  void setEditorData(QWidget *editor, const QModelIndex &index) const override;
  void setModelData(QWidget *editor, QAbstractItemModel *model,
                    const QModelIndex &index) const override;

  QSize sizeHint(const QStyleOptionViewItem &,
                 const QModelIndex &) const override;
};
