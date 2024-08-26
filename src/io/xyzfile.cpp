#include "xyzfile.h"
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <occ/core/element.h>

XYZFile::XYZFile(const std::vector<QString> &atomSymbols,
                 Eigen::Ref<const occ::Mat3N> atomPositions) {
  setAtomSymbols(atomSymbols);
  setAtomPositions(atomPositions);
}

bool XYZFile::readFromFile(const QString &fileName) {
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning() << "Unable to open file:" << fileName;
    return false;
  }

  QTextStream in(&file);
  QString content = in.readAll();
  file.close();
  return readFromString(content);
}

bool XYZFile::readFromString(const QString &content) {
  QString contentCopy = content;
  QTextStream in(&contentCopy, QIODevice::ReadOnly);

  bool ok;
  int atomCount;

  // Read the first line and convert it to an integer
  QString line = in.readLine();
  if (!line.isNull()) {
    atomCount = line.toInt(&ok);
    if (!ok || atomCount <= 0) {
      qWarning() << "Invalid atom count!";
      return false;
    }
  } else {
    qWarning() << "Invalid .xyz format!";
    return false;
  }

  // Read and skip the second line
  m_comment = in.readLine().trimmed();

  // Clear previous data
  m_atomSymbols.clear();
  m_atomPositions.clear();

  int atomReadCount = 0;
  while (!in.atEnd()) {
    line = in.readLine().trimmed();

    if (line.isEmpty()) {
      continue;
    }

    QStringList lineData =
        line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (lineData.size() != 4) {
      qWarning() << "Invalid line format!";
      return false;
    }

    m_atomSymbols.push_back(lineData[0]);

    float x = lineData[1].toFloat(&ok);
    if (!ok) {
      qWarning() << "Invalid x coordinate!";
      return false;
    }

    float y = lineData[2].toFloat(&ok);
    if (!ok) {
      qWarning() << "Invalid y coordinate!";
      return false;
    }

    float z = lineData[3].toFloat(&ok);
    if (!ok) {
      qWarning() << "Invalid z coordinate!";
      return false;
    }

    m_atomPositions.push_back({x, y, z});

    ++atomReadCount;
    if (atomReadCount == atomCount) {
      break;
    }
  }

  if (atomReadCount != atomCount) {
    qWarning() << "Invalid .xyz format!";
    return false;
  }

  return true;
}

void XYZFile::setAtomSymbols(const std::vector<QString> &symbols) {
  m_atomSymbols = symbols;
}

const std::vector<QString> &XYZFile::getAtomSymbols() const {
  return m_atomSymbols;
}

void XYZFile::setElements(Eigen::Ref<const occ::IVec> nums) {
  m_atomSymbols.resize(nums.rows());
  for (int i = 0; i < nums.rows(); i++) {
    m_atomSymbols[i] =
        QString::fromStdString(occ::core::Element(nums(i)).symbol());
  }
}

const std::vector<occ::Vec3> &XYZFile::getAtomPositions() const {
  return m_atomPositions;
}

void XYZFile::setAtomPositions(const std::vector<occ::Vec3> &pos) {
  m_atomPositions = pos;
}

void XYZFile::setAtomPositions(Eigen::Ref<const occ::Mat3N> pos) {
  m_atomPositions.resize(pos.cols());
  for (int i = 0; i < pos.cols(); i++) {
    m_atomPositions[i] = pos.col(i);
  }
}

const QString &XYZFile::getComment() const { return m_comment; }

void XYZFile::setComment(const QString &comment) { m_comment = comment; }

bool XYZFile::writeToFile(const QString &filename) const {
  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    qDebug() << "Could not open file for writing in XYZFile::writeToFile";
    return false;
  }

  QString contents = toString();

  if (contents.isEmpty())
    return false;
  QTextStream out(&file);
  out << contents;

  file.close();
  qDebug() << "Wrote xyz file to" << QFileInfo(file).absolutePath() << filename;
  return true;
}

QString XYZFile::toString() const {
  if (m_atomSymbols.size() != m_atomPositions.size()) {
    qDebug() << "Invalid write xyz call, mismatch in nums and positions size";
    return "";
  }

  QString result;

  QTextStream out(&result);

  out << m_atomPositions.size() << "\n";
  out << m_comment << "\n";

  for (int i = 0; i < m_atomPositions.size(); ++i) {
    const auto &pos = m_atomPositions[i];
    out << m_atomSymbols[i] << " " << pos(0) << " " << pos(1) << " " << pos(2)
        << "\n";
  }
  return result;
}

bool TrajFile::readFromFile(const QString &fileName) {
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning() << "Unable to open file:" << fileName;
    return false;
  }

  QTextStream in(&file);
  QString content = in.readAll();
  file.close();
  return readFromString(content);
}

bool TrajFile::readFromString(const QString &content) {
  QStringList lines = content.split('\n');
  qDebug() << "Found" << lines.size() << "lines in file";

  for (int i = 0; i < lines.size();) {
    if (i + 1 >= lines.size())
      break;

    int numAtoms = lines[i].toInt();
    qDebug() << numAtoms << "atoms";
    if (numAtoms <= 0 || i + numAtoms + 1 >= lines.size())
      break;

    QString frameContent = lines.mid(i, numAtoms + 2).join('\n');
    XYZFile frame;
    if (!frame.readFromString(frameContent)) {
      return false;
    }
    m_frames.append(frame);

    i += numAtoms + 2;
  }

  if (m_frames.isEmpty()) {
    return false;
  }

  return true;
}
