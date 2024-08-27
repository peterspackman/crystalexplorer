#include "exportdialog.h"
#include "ui_exportdialog.h"
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QResizeEvent>

ExportDialog::ExportDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::ExportDialog) {
  ui->setupUi(this);
  QStringList resolutionOptions{"1x", "2x", "3x", "4x"};
  ui->resolutionScaleComboBox->addItems(resolutionOptions);
  ui->resolutionScaleComboBox->setCurrentText("1x");
  initConnections();
}

ExportDialog::~ExportDialog() { delete ui; }

void ExportDialog::initConnections() {
  connect(ui->destinationBrowseButton, &QPushButton::clicked, this,
          &ExportDialog::selectFile);
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(ui->resolutionScaleComboBox, &QComboBox::currentIndexChanged, this,
          &ExportDialog::updateResolutionLabel);
  connect(ui->backgroundColorToolButton, &QAbstractButton::clicked, [&]() {
    QColor color = QColorDialog::getColor(currentBackgroundColor(), this,
                                          "Set Background Color for Export",
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
      updateBackgroundColor(color);
    }
  });
}

int ExportDialog::currentResolutionScale() const {
  QString currentText = ui->resolutionScaleComboBox->currentText();
  // Remove the 'x' at the end of the string
  currentText = currentText.remove(currentText.length() - 1, 1);
  bool conversionOk{false};
  int scale = currentText.toInt(&conversionOk);

  if (!conversionOk) {
    qWarning() << "Failed to convert resolution scale text to float: "
               << currentText;
    return 1; // Default to 1x if conversion fails
  }
  return scale;
}

void ExportDialog::updateResolutionLabel() {
  int scale = currentResolutionScale();
  if (m_currentPixmap.isNull()) {
    ui->resolutionLabel->setText("N/A");
    return;
  }
  QSize originalSize = m_currentPixmap.size();

  int newWidth = originalSize.width() * scale;
  int newHeight = originalSize.height() * scale;

  QString resolutionText = QString("%1 x %2 px").arg(newWidth).arg(newHeight);

  ui->resolutionLabel->setText(resolutionText);
}

void ExportDialog::updateFilePath(QString path) {
  m_currentFilePath = path;
  ui->destinationLineEdit->setText(path);
}

QColor ExportDialog::currentBackgroundColor() const {
  return m_currentBackgroundColor;
}

void ExportDialog::updateBackgroundColor(const QColor &color) {
  m_currentBackgroundColor = color;
  auto *button = ui->backgroundColorToolButton;
  QPixmap pixmap = QPixmap(button->iconSize());
  pixmap.fill(color);
  button->setIcon(QIcon(pixmap));
}

void ExportDialog::selectFile() {
  QString filter = "Portable Network Graphics (*.png);; POV-ray (*.pov)";
  QString filePath = QFileDialog::getSaveFileName(this, tr("Export graphics"),
                                                  m_currentFilePath, filter);

  if (!filePath.isEmpty()) {
    m_currentFilePath = filePath;
    updateFilePath(filePath);
    updatePreview();
  }
}

void ExportDialog::updateImage(const QImage &image) {
  bool success = m_currentPixmap.convertFromImage(image);
  qDebug() << "Loaded pixmap from image";
  updatePreview();
}

void ExportDialog::resizeEvent(QResizeEvent *event) {
  QDialog::resizeEvent(event);
  qDebug() << "Dialog resized to:" << event->size();
  updatePreview();
}

void ExportDialog::updatePreview() {
  auto *label = ui->pixmapDisplayLabel;

  qDebug() << "Dialog size:" << this->size();
  qDebug() << "Label size:" << label->size();
  qDebug() << "Label minimum size:" << label->minimumSize();
  qDebug() << "Label maximum size:" << label->maximumSize();
  qDebug() << "Label size policy:" << label->sizePolicy().horizontalPolicy()
           << label->sizePolicy().verticalPolicy();

  if (m_currentPixmap.isNull()) {
    qDebug() << "Current pixmap is null";
    label->setText("No image to preview");
    return;
  }

  QSize labelSize = label->size();
  QPixmap scaledPixmap = m_currentPixmap.scaled(labelSize, Qt::KeepAspectRatio,
                                                Qt::SmoothTransformation);

  QPixmap background(labelSize);
  background.fill(QColor::fromRgb(255, 255, 255, 0));

  // Create a painter to draw on the background
  QPainter painter(&background);

  // Calculate position to center the scaled pixmap
  int x = (labelSize.width() - scaledPixmap.width()) / 2;
  int y = (labelSize.height() - scaledPixmap.height()) / 2;

  // Draw the scaled pixmap onto the background
  painter.drawPixmap(x, y, scaledPixmap);

  // Set the final composited pixmap to the label
  label->setPixmap(background);
  qDebug() << "Original pixmap size:" << m_currentPixmap.size();
  qDebug() << "Scaled pixmap size:" << scaledPixmap.size();
  updateResolutionLabel();
}

void ExportDialog::accept() {
  if (m_currentFilePath.isEmpty()) {
    QMessageBox::warning(this, "Warning", "Please select a destination file.");
    return; // Don't accept yet
  }

  QDialog::accept();
}

void ExportDialog::reject() { QDialog::reject(); }
