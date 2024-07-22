#include "colordelegate.h"
#include <QColorDialog>
#include <QPainter>

ColorDelegate::ColorDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void ColorDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const {
  QColor color = index.model()->data(index, Qt::DisplayRole).value<QColor>();
  painter->fillRect(option.rect, color);
}

QWidget *ColorDelegate::createEditor(QWidget *parent,
                                     const QStyleOptionViewItem &,
                                     const QModelIndex &) const {
  QColorDialog *dialog = new QColorDialog(parent);
  dialog->setOption(QColorDialog::ShowAlphaChannel);
  dialog->setModal(true);
  return dialog;
}

void ColorDelegate::setEditorData(QWidget *editor,
                                  const QModelIndex &index) const {
  QColor color = index.model()->data(index, Qt::DisplayRole).value<QColor>();
  static_cast<QColorDialog *>(editor)->setCurrentColor(color);
}

void ColorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                 const QModelIndex &index) const {
  QColor color = static_cast<QColorDialog *>(editor)->currentColor();
  model->setData(index, QVariant::fromValue(color));
}

QSize ColorDelegate::sizeHint(const QStyleOptionViewItem &,
                              const QModelIndex &) const {
  return QSize(50, 20);
}
