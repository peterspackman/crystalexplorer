#pragma once
#include <QDialog>
#include <QImage>

namespace Ui {
class ExportDialog;
}

class ExportDialog : public QDialog {
  Q_OBJECT

public:
  explicit ExportDialog(QWidget *parent = nullptr);
  ~ExportDialog();

  QString currentFilePath() const { return m_currentFilePath; }
  int currentResolutionScale() const;
  QColor currentBackgroundColor() const;

public slots:
  void updateImage(const QImage &);
  void updateFilePath(QString);
  void updateBackgroundColor(const QColor &);

  void accept() override;
  void reject() override;

private slots:
  void updateResolutionLabel();
  void selectFile();
  void updatePreview();

protected:
  void resizeEvent(QResizeEvent *event) override;

private:
  QString m_currentFilePath{"destination.png"};
  QPixmap m_currentPixmap;
  QColor m_currentBackgroundColor{Qt::white};
  void initConnections();
  Ui::ExportDialog *ui;
};
