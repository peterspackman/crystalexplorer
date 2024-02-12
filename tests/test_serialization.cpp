#include <QtTest>
#include <QByteArray>
#include "settings.h"
#include "atom.h"
#include "surface.h"
#include "elementdata.h"




class TestSerialization: public QObject {
    Q_OBJECT

public:
    TestSerialization() {
	Q_INIT_RESOURCE(crystalexplorer);
	Q_INIT_RESOURCE(mesh);
    }


private:
    template <typename T>
    QByteArray serialize(const T &object);

    template <typename T>
    int deserialize(const QByteArray &data, T &object);


private slots:
    void initTestCase() {
      QString filename =
	  settings::readSetting(settings::keys::ELEMENTDATA_FILE).toString();
      qDebug() << "Filename" << filename;
      QFileInfo fileInfo(filename);
      qDebug() << "Exists:" << fileInfo.exists();
      bool useJmolColors =
	  settings::readSetting(settings::keys::USE_JMOL_COLORS).toBool();

      bool success = ElementData::getData(filename, useJmolColors);
      qDebug() << "successfully read element data:" << success;

    }

    void testAtom();
    void testSurface();
};


template <typename T>
QByteArray TestSerialization::serialize(const T &object) {
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    QDataStream out(&buffer);
    out << object;
    buffer.close();
    return data;
}

template <typename T>
int TestSerialization::deserialize(const QByteArray &data, T &object) {
    QBuffer buffer(const_cast<QByteArray*>(&data));
    buffer.open(QIODevice::ReadOnly);
    QDataStream in(&buffer);
    in >> object;
    return 0;
}

void TestSerialization::testAtom() {
    Atom atom("atom label 1", "Ba", 0.0, 0.5, 100.0,
	      7, 0.8);

    Atom atom2("atom label 2", "H", 1.0, 0.0, 0.5,
	      1, 1.8);

    auto data = serialize(atom);
    int n = deserialize(data, atom2);

    QCOMPARE(atom.label(), atom2.label());
    QCOMPARE(atom.pos(), atom2.pos());
    QCOMPARE(atom.disorderGroup(), atom2.disorderGroup());
    QCOMPARE(atom.occupancy(), atom2.occupancy());
}

void TestSerialization::testSurface() {
    Surface s;
    s.addVertex(0.5, 0.5, 0.5);
    s.addVertex(0.5, 0.5, -0.5);
    s.addVertex(0.5, -0.5, 0.5);
    s.addVertex(0.5, -0.5, -0.5);
    s.addVertex(-0.5, 0.5, 0.5);
    s.addVertex(-0.5, 0.5, -0.5);
    s.addVertex(-0.5, -0.5, 0.5);
    s.addVertex(-0.5, -0.5, -0.5);

    // Add faces (triangles), with vertices indexed from 0
    s.addFace(0, 2, 4);
    s.addFace(2, 6, 4);
    s.addFace(1, 5, 3);
    s.addFace(3, 5, 7);
    s.addFace(0, 4, 1);
    s.addFace(1, 4, 5);
    s.addFace(2, 3, 6);
    s.addFace(3, 7, 6);
    s.addFace(0, 1, 2);
    s.addFace(1, 3, 2);
    s.addFace(4, 6, 5);
    s.addFace(5, 6, 7);
    s.update();

    qDebug() << "Surface volume: " << s.volume();

    Surface s2;

    auto data = serialize(s);
    int n = deserialize(data, s2);

    QCOMPARE(s.surfaceName(), s2.surfaceName());
    QCOMPARE(s.volume(), s2.volume());
}


QTEST_MAIN(TestSerialization)
#include "test_serialization.moc"
