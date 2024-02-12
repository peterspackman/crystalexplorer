#include <QElapsedTimer>
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QtDebug>
#include <QRegularExpression>

#include "settings.h"
#include "surface.h"
#include "surfacedata.h"

#include "qeigen.h"
#include "sbf.hpp"

template <typename OurType, typename SBFType>
QVector<OurType> read_prop_from_sbf(sbf::File &file, const char *dataset_name) {
  auto dset = file.get_dataset(dataset_name);
  if (dset.is_empty()) {
    qDebug() << "Could not find dataset:" << dataset_name;
    return {};
  }
  auto shape = dset.get_shape();
  std::vector<SBFType> buffer(shape[0]);
  auto success = file.read_data<SBFType>(dset.name(), buffer.data());
  if (success != sbf::success) {
    qDebug() << "Error reading data into buffer";
    return {};
  }
  QVector<OurType> propertyValues;
  propertyValues.reserve(shape[0]);
  std::copy(buffer.begin(), buffer.end(), std::back_inserter(propertyValues));
  return propertyValues;
}

SurfacePropertyProxy
SurfaceData::getRequestedPropertyData(const JobParameters &jobParams) {
  SurfacePropertyProxy property;

  QString propertyString =
      IsosurfacePropertyDetails::getAttributes(jobParams.requestedPropertyType)
          .tontoName;
  property.first = propertyString;
  if (jobParams.requestedPropertyType == IsosurfacePropertyDetails::Type::None)
    return property;

  bool use_sbf =
      settings::readSetting(settings::keys::USE_SBF_INTERFACE).toBool();

  if (use_sbf) {
    auto fileName = jobParams.outputFilename.toUtf8();
    sbf::File file(fileName.constData(), sbf::reading);
    qDebug() << "Trying to read" << propertyString << "from SBF file";
    auto propertyNameUtf8 = propertyString.toUtf8();
    property.second = read_prop_from_sbf<float, sbf::sbf_double>(
        file, propertyNameUtf8.constData());
    file.close();
    if (settings::readSetting(settings::keys::DELETE_WORKING_FILES).toBool()) {
      QFile file(jobParams.outputFilename);
      file.remove();
    }
  } else {
    QFile file(jobParams.outputFilename);
    if (file.open(QIODevice::ReadOnly)) {

      QTextStream ts(&file);

      while (!ts.atEnd()) {
        QStringList tokens = ts.readLine().split(QRegularExpression("\\s+"));
        if (tokens.size() < 2) {
          continue;
        }

        QString blockDelimiter = tokens[0];
        if (blockDelimiter != "begin") {
          continue;
        }

        QString blockType = tokens[1];

        if (blockType == propertyString) {

          int nExpectedValues = tokens[2].toInt();

          QVector<float> propertyValues =
              processPropertyData(ts, propertyString);

          int nValues = propertyValues.size();
          if (nValues > 0 && nValues == nExpectedValues) {
            property.second = propertyValues;
            break;
          }
        }
      }
    }
    if (settings::readSetting(settings::keys::DELETE_WORKING_FILES).toBool()) {
      file.remove();
    }
  }
  return property;
}

Surface *SurfaceData::getData(const JobParameters &jobParams) {
  Surface *surface = nullptr;
  bool use_sbf =
      settings::readSetting(settings::keys::USE_SBF_INTERFACE).toBool();
  if (use_sbf) {
    surface = new Surface();
    sbf::File file(jobParams.outputFilename.toUtf8().constData(), sbf::reading);
    readSurface(file, surface);
    readSurfaceProperties(file, surface);
    file.close();

    surface->postReadingInit(jobParams);
    if (settings::readSetting(settings::keys::DELETE_WORKING_FILES).toBool()) {
      QFile file(jobParams.outputFilename);
      file.remove();
    }

  } else {
    QFile file(jobParams.outputFilename);
    if (file.open(QIODevice::ReadOnly)) {

      QTextStream ts(&file);

      while (!ts.atEnd()) {
        QStringList tokens = ts.readLine().split(QRegularExpression("\\s+"));
        if (tokens.size() < 2) {
          continue;
        }

        QString blockDelimiter = tokens[0];
        if (blockDelimiter != "begin") {
          continue;
        }

        QString blockType = tokens[1];

        if (blockType == "surface") {
          surface = processSurfaceBlock(ts);
          if (surface != nullptr) {
            surface->postReadingInit(jobParams);
          }
        }
      }
    }
    if (settings::readSetting(settings::keys::DELETE_WORKING_FILES).toBool()) {
      file.remove();
    }
  }
  if (surface != nullptr) {
    if (surface->type() == IsosurfaceDetails::Type::CrystalVoid) {
      surface->setFrontFace(GL_CW);
      surface->flipVertexNormals();
    }
  }
  return surface;
}

template <typename OurType, typename SBFType, class CallBack>
void process_matrix_from_sbf(sbf::File &file, const char *dataset_name,
                             CallBack &callback) {
  auto dset = file.get_dataset(dataset_name);
  auto shape = dset.get_shape();
  std::vector<SBFType> buffer(shape[0] * shape[1]);
  auto success = file.read_data<SBFType>(dset.name(), buffer.data());
  if (success != sbf::success) {
    qDebug() << "Error reading dataset: " << dataset_name;
    // TODO handle error
  }
  callback(buffer, shape);
  qDebug() << "Successfully read dataset: " << dataset_name;
}

bool SurfaceData::readSurface(sbf::File &file, Surface *surface) {

  auto add_vertices = [&surface](const std::vector<double> &buffer,
                                 const sbf::sbf_dimensions &shape) {
    auto data =
        Eigen::Map<const Eigen::MatrixXd>(buffer.data(), shape[0], shape[1]);
    for (Eigen::Index i = 0; i < data.cols(); i++)
      surface->addVertex(data(0, i), data(1, i), data(2, i));
  };
  process_matrix_from_sbf<double, sbf::sbf_double>(file, "vertices",
                                                   add_vertices);

  auto add_faces = [&surface](const std::vector<int> &buffer,
                              const sbf::sbf_dimensions &shape) {
    auto data =
        Eigen::Map<const Eigen::MatrixXi>(buffer.data(), shape[0], shape[1]);
    for (Eigen::Index i = 0; i < data.cols(); i++)
      surface->addFace(data(0, i) - 1, data(1, i) - 1, data(2, i) - 1);
  };
  process_matrix_from_sbf<int, sbf::sbf_integer>(file, "faces", add_faces);

  auto add_normals = [&surface](const std::vector<double> &buffer,
                                const sbf::sbf_dimensions &shape) {
    auto data =
        Eigen::Map<const Eigen::MatrixXd>(buffer.data(), shape[0], shape[1]);
    for (Eigen::Index i = 0; i < data.cols(); i++)
      surface->addVertexNormal(data(0, i), data(1, i), data(2, i));
  };
  process_matrix_from_sbf<double, sbf::sbf_double>(file, "vertex normals",
                                                   add_normals);

  auto add_atoms_inside = [&surface](const std::vector<int> &buffer,
                                     const sbf::sbf_dimensions &shape) {
    auto data =
        Eigen::Map<const Eigen::MatrixXi>(buffer.data(), shape[0], shape[1]);
    for (Eigen::Index i = 0; i < data.rows(); i++)
      surface->addInsideAtom(data(i, 0) - 1, data(i, 1), data(i, 2),
                             data(i, 3));
  };
  process_matrix_from_sbf<int, sbf::sbf_integer>(file, "atoms_inside_surface",
                                                 add_atoms_inside);

  auto add_atoms_outside = [&surface](const std::vector<int> &buffer,
                                      const sbf::sbf_dimensions &shape) {
    auto data =
        Eigen::Map<const Eigen::MatrixXi>(buffer.data(), shape[0], shape[1]);
    for (Eigen::Index i = 0; i < data.rows(); i++)
      surface->addOutsideAtom(data(i, 0) - 1, data(i, 1), data(i, 2),
                              data(i, 3));
  };
  process_matrix_from_sbf<int, sbf::sbf_integer>(file, "atoms_outside_surface",
                                                 add_atoms_outside);
  return true;
}

bool SurfaceData::readSurfaceProperties(sbf::File &file, Surface *surface) {
  for (const auto &prop :
       std::as_const(IsosurfacePropertyDetails::getAvailableTypes())) {
    auto propertyName = prop.tontoName.toUtf8();
    qDebug() << "Property Name: " << propertyName;
    auto property =
        read_prop_from_sbf<float, sbf::sbf_double>(file, propertyName);
    if (!property.empty()) {
      surface->addProperty(propertyName, property);
    }
  }
  auto de_atoms = read_prop_from_sbf<int, sbf::sbf_integer>(file, "d_e_atoms");
  if (!de_atoms.empty()) {
    for (const auto &x : de_atoms) {
      surface->addDeFaceAtom(x - 1);
    }
  }
  auto di_atoms = read_prop_from_sbf<int, sbf::sbf_integer>(file, "d_i_atoms");
  if (!di_atoms.empty()) {
    for (const auto &x : di_atoms) {
      surface->addDiFaceAtom(x - 1);
    }
  }
  return true;
}

Surface *SurfaceData::processSurfaceBlock(QTextStream &ts) {
  Surface *surface = new Surface();

  bool successfullyReadVertices = false;
  bool successfullyReadIndices = false;
  bool successfullyReadVertexNormals = false;
  bool successfullyReadVertexProperties = false;
  bool successfullyReadAtomsInsideSurface = false;
  bool successfullyReadAtomsOutsideSurface = false;
  bool successfullyReadDiFaceAtoms = false;
  bool successfullyReadDeFaceAtoms = false;

  while (!ts.atEnd()) {
    QStringList tokens =
        ts.readLine().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (tokens.size() < 2) {
      continue;
    }

    QString blockDelimiter = tokens[0];
    QString blockType = tokens[1];

    if (blockDelimiter == "end" && blockType == "surface") {
      break;
    }

    if (blockDelimiter == "begin" && blockType == "vertices") {
      successfullyReadVertices =
          processVerticesBlock(ts, surface, tokens[2].toInt());
    }
    if (blockDelimiter == "begin" && blockType == "indices") {
      if (tokens.size() == 4) {
        surface->setNumberOfCaps(tokens[3].toInt());
      }
      successfullyReadIndices =
          processIndicesBlock(ts, surface, tokens[2].toInt());
    }
    if (blockDelimiter == "begin" && blockType == "vertex_normals") {
      successfullyReadVertexNormals =
          processVertexNormalsBlock(ts, surface, tokens[2].toInt());
    }
    if (blockDelimiter == "begin" && blockType == "vertex_properties") {
      successfullyReadVertexProperties =
          processVertexPropertiesBlock(ts, surface);
    }
    if (blockDelimiter == "begin" && blockType == "atoms_inside_surface") {
      successfullyReadAtomsInsideSurface =
          processAtomsInsideSurfaceBlock(ts, surface, tokens[2].toInt());
    }
    if (blockDelimiter == "begin" && blockType == "atoms_outside_surface") {
      successfullyReadAtomsOutsideSurface =
          processAtomsOutsideSurfaceBlock(ts, surface, tokens[2].toInt());
    }
    if (blockDelimiter == "begin" && blockType == "d_i_face_atoms") {
      successfullyReadDiFaceAtoms =
          processDiFaceAtoms(ts, surface, tokens[2].toInt());
    }
    if (blockDelimiter == "begin" && blockType == "d_e_face_atoms") {
      successfullyReadDeFaceAtoms =
          processDeFaceAtoms(ts, surface, tokens[2].toInt());
    }
  }

  if (successfullyReadVertices && successfullyReadIndices &&
      successfullyReadVertexNormals && successfullyReadVertexProperties &&
      successfullyReadAtomsInsideSurface &&
      successfullyReadAtomsOutsideSurface && successfullyReadDiFaceAtoms &&
      successfullyReadDeFaceAtoms) {
    return surface;
  } else {
    delete surface;
    return nullptr;
  }
}

bool SurfaceData::processVerticesBlock(QTextStream &ts, Surface *surface,
                                       int nExpectedVertices) {
  int nVertices = 0;
  while (!ts.atEnd()) {
    QString line = ts.readLine();
    QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (tokens.size() < 2) {
      continue;
    }

    QString blockDelimiter = tokens[0];
    QString blockType = tokens[1];

    if (blockDelimiter == "end" && blockType == "vertices") {
      break;
    }

    Q_ASSERT(tokens.size() == 3);

    surface->addVertex(tokens[0].toFloat(), tokens[1].toFloat(),
                       tokens[2].toFloat());
    nVertices++;
  }
  return (nVertices > 0 && nVertices == nExpectedVertices);
}

bool SurfaceData::processIndicesBlock(QTextStream &ts, Surface *surface,
                                      int nExpectedFaces) {
  Q_UNUSED(nExpectedFaces);

  int nFaces = 0;
  while (!ts.atEnd()) {
    QString line = ts.readLine();
    QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (tokens.size() < 2) {
      continue;
    }

    QString blockDelimiter = tokens[0];
    QString blockType = tokens[1];

    if (blockDelimiter == "end" && blockType == "indices") {
      break;
    }

    Q_ASSERT(tokens.size() == 3);
    surface->addFace(tokens[0].toInt(), tokens[1].toInt(), tokens[2].toInt());
    nFaces++;
  }
  return (nFaces > 0);
}

bool SurfaceData::processVertexNormalsBlock(QTextStream &ts, Surface *surface,
                                            int nExpectedNormals) {
  int nNormals = 0;
  while (!ts.atEnd()) {
    QString line = ts.readLine();
    QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (tokens.size() < 2) {
      continue;
    }

    QString blockDelimiter = tokens[0];
    QString blockType = tokens[1];

    if (blockDelimiter == "end" && blockType == "vertex_normals") {
      break;
    }

    Q_ASSERT(tokens.size() == 3);
    surface->addVertexNormal(tokens[0].toFloat(), tokens[1].toFloat(),
                             tokens[2].toFloat());
    nNormals++;
  }
  return (nNormals > 0 && nNormals == nExpectedNormals);
}

bool SurfaceData::processVertexPropertiesBlock(QTextStream &ts,
                                               Surface *surface) {
  bool successfullyReadProperties = true;
  int nProperties = 0;

  QString blockTypeForWarning;

  while (!ts.atEnd()) {
    QString line = ts.readLine();
    QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (tokens.size() < 2) {
      continue;
    }

    QString blockDelimiter = tokens[0];
    QString blockType = tokens[1];

    if (blockDelimiter == "end" && blockType == "vertex_properties") {
      successfullyReadProperties = successfullyReadProperties && true;
      break;
    }

    blockTypeForWarning = blockType;

    Q_ASSERT(tokens.size() == 3);
    if (blockDelimiter == "begin") {
      bool success = processProperty(ts, surface, blockType, tokens[2].toInt());
      successfullyReadProperties = successfullyReadProperties && success;
      nProperties++;
    }
  }
  return successfullyReadProperties;
}

bool SurfaceData::processProperty(QTextStream &ts, Surface *surface,
                                  QString propertyString, int nExpectedValues) {
  QVector<float> propertyValues = processPropertyData(ts, propertyString);

  int nValues = propertyValues.size();

  if (nValues > 0) {
    surface->addProperty(propertyString, propertyValues);
  }

  return (nValues == nExpectedValues);
}

QVector<float> SurfaceData::processPropertyData(QTextStream &ts,
                                                QString propertyString) {
  QVector<float> propertyValues;

  while (!ts.atEnd()) {
    QString line = ts.readLine();
    QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (tokens.size() == 0) {
      continue;
    }

    if (tokens.size() == 2) {
      QString blockDelimiter = tokens[0];
      QString blockType = tokens[1];

      if (blockDelimiter == "end" && blockType == propertyString) {
        break;
      }
    }

    propertyValues.append(tokens[0].toFloat());
  }

  return propertyValues;
}

bool SurfaceData::processAtomsInsideSurfaceBlock(QTextStream &ts,
                                                 Surface *surface,
                                                 int nExpectedAtoms) {
  int nAtoms = 0;
  while (!ts.atEnd()) {
    QString line = ts.readLine();
    QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (tokens.size() < 2) {
      continue;
    }

    QString blockDelimiter = tokens[0];
    QString blockType = tokens[1];

    if (blockDelimiter == "end" && blockType == "atoms_inside_surface") {
      break;
    }

    Q_ASSERT(tokens.size() == 4);
    surface->addInsideAtom(tokens[0].toInt() - 1, tokens[1].toInt(),
                           tokens[2].toInt(), tokens[3].toInt());
    nAtoms++;
  }
  return (nAtoms == nExpectedAtoms);
}

bool SurfaceData::processAtomsOutsideSurfaceBlock(QTextStream &ts,
                                                  Surface *surface,
                                                  int nExpectedAtoms) {
  int nAtoms = 0;
  while (!ts.atEnd()) {
    QString line = ts.readLine();
    QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (tokens.size() < 2) {
      continue;
    }

    QString blockDelimiter = tokens[0];
    QString blockType = tokens[1];

    if (blockDelimiter == "end" && blockType == "atoms_outside_surface") {
      break;
    }

    Q_ASSERT(tokens.size() == 4);
    surface->addOutsideAtom(tokens[0].toInt() - 1, tokens[1].toInt(),
                            tokens[2].toInt(), tokens[3].toInt());
    nAtoms++;
  }
  return (nAtoms == nExpectedAtoms);
}

bool SurfaceData::processDiFaceAtoms(QTextStream &ts, Surface *surface,
                                     int nExpectedAtoms) {
  int nAtoms = 0;
  while (!ts.atEnd()) {
    QString line = ts.readLine();
    QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (tokens.size() == 2) {
      QString blockDelimiter = tokens[0];
      QString blockType = tokens[1];

      if (blockDelimiter == "end" && blockType == "d_i_face_atoms") {
        break;
      }
    }

    Q_ASSERT(tokens.size() == 1);
    surface->addDiFaceAtom(tokens[0].toInt() - 1);
    nAtoms++;
  }
  return (nAtoms == nExpectedAtoms);
}

bool SurfaceData::processDeFaceAtoms(QTextStream &ts, Surface *surface,
                                     int nExpectedAtoms) {
  int nAtoms = 0;
  while (!ts.atEnd()) {
    QString line = ts.readLine();
    QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (tokens.size() == 2) {
      QString blockDelimiter = tokens[0];
      QString blockType = tokens[1];

      if (blockDelimiter == "end" && blockType == "d_e_face_atoms") {
        break;
      }
    }

    Q_ASSERT(tokens.size() == 1);
    surface->addDeFaceAtom(tokens[0].toInt() - 1);
    nAtoms++;
  }
  return (nAtoms == nExpectedAtoms);
}
