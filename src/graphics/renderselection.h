#pragma once
#include <QColor>
#include <QObject>
#include <QVector3D>
#include <ankerl/unordered_dense.h>
#include <cstdint>

namespace cx::graphics {

enum class SelectionType : uint8_t {
  None = 0,
  Atom = 1,
  Bond = 2,
  Surface = 3,
  Aggregate = 4,
  MaxType = 7 // Maximum of 8 types (3 bits)
};

struct GraphicsIdentifier {
  uint8_t kind{0};
  uint32_t value{0};
  uint32_t secondary{0};

  GraphicsIdentifier() = default;
  GraphicsIdentifier(SelectionType kind, uint32_t v, uint32_t s = 0);

  inline SelectionType selectionType() const {
    return static_cast<SelectionType>(kind);
  }

  uint32_t encode() const;
  QColor toColor() const;

  static GraphicsIdentifier decode(uint32_t);
  static GraphicsIdentifier fromColor(const QColor &);
  QString toString() const;
};

struct SelectionResult {
  SelectionType type{SelectionType::None};
  quint32 identifier{0};
  int index{-1};
  quint32 secondaryIndex{0};
};

class RenderSelection : public QObject {
  Q_OBJECT
public:
  using IdentifierIndexMap = ankerl::unordered_dense::map<quint32, int>;
  using IndexIdentifierMap = ankerl::unordered_dense::map<int, quint32>;

  RenderSelection(QObject *parent = nullptr);

  [[nodiscard]] SelectionResult getSelectionFromColor(const QColor &) const;

  void clear(SelectionType);
  void clear();

  quint32 add(SelectionType type, int index);
  [[nodiscard]] QVector3D getColorFromId(quint32) const;

private:

  std::vector<IdentifierIndexMap> m_indexMaps;
  std::vector<IndexIdentifierMap> m_identifierMaps;
};

} // namespace cx::graphics
