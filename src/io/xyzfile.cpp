#include "xyzfile.h"
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

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
  in.readLine();

  // Clear previous data
  m_atomSymbols.clear();
  m_atomPositions.clear();

  int atomReadCount = 0;
  while (!in.atEnd()) {
    line = in.readLine().trimmed();

    if (line.isEmpty()) {
      continue;
    }

    QStringList lineData = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
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

const std::vector<QString> &XYZFile::getAtomSymbols() const {
  return m_atomSymbols;
}

const std::vector<occ::Vec3> &XYZFile::getAtomPositions() const {
  return m_atomPositions;
}
