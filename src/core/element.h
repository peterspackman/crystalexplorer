#pragma once
#include <QColor>
#include <QDataStream>
#include <QString>
#include <vector>

class Element {
  friend QDataStream &operator<<(QDataStream &, const Element &);
  friend QDataStream &operator>>(QDataStream &, Element &);

public:
  Element(const QString &, const QString &, int, float, float, float,
          const QColor &);
  void update(const QString &, const QString &, int, float, float, float,
              const QColor &); // Should only be used by ElementData

  inline float covRadius() const { return _covRadius; }
  inline float vdwRadius() const { return _vdwRadius; }
  inline float mass() const { return _mass; }
  inline const QString &name() const { return _name; }
  inline const QColor &color() const { return _color; }
  inline const QString &symbol() const { return _symbol; }
  QString capitalizedSymbol() const;
  inline int number() const { return _number; }

  inline void setCovRadius(float covRadius) { _covRadius = covRadius; }
  inline void setVdwRadius(float vdwRadius) { _vdwRadius = vdwRadius; }
  inline void setColor(const QColor &color) { _color = color; }

private:
  QString _name;
  QString _symbol;
  int _number;
  float _covRadius;
  float _vdwRadius;
  float _mass;
  QColor _color;
};

QString formulaSum(const std::vector<QString> &elements, bool richText = false);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &, const Element &);
QDataStream &operator>>(QDataStream &, Element &);
