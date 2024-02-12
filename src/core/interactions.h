#pragma once
#include <QObject>
#include <ankerl/unordered_dense.h>

struct DimerPair {
    int firstIndex{0};
    int secondIndex{0};

    // Make sure they're ordered (first < second)
    DimerPair(int a, int b)
        : firstIndex(a < b ? a : b),
          secondIndex(a < b ? b : a) {}

    bool operator==(const DimerPair& other) const {
        return firstIndex == other.firstIndex && secondIndex == other.secondIndex;
    }
};

struct DimerPairHash {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(DimerPair const &i) const noexcept -> uint64_t {
        static_assert(std::has_unique_object_representations_v<DimerPair>);
        return ankerl::unordered_dense::detail::wyhash::hash(&i, sizeof(i));
    }
};

class DimerInteractions: public QObject {
Q_OBJECT
public:
    using Components = ankerl::unordered_dense::map<QString, double>;
    using DimerInteractionValues = ankerl::unordered_dense::map<DimerPair, Components, DimerPairHash>;
    constexpr static double DefaultValue = 0.0;

    DimerInteractions(QObject *parent=nullptr);


    void clear();
    void clearValue(DimerPair pair, const QString &label);
    void clearValues(DimerPair pair);

    void setValue(DimerPair pair, double value, const QString &label);
    void setValues(DimerPair pair, Components &&components);
    
    [[nodiscard]] double valueForDimer(DimerPair pair, const QString &label) const;
    [[nodiscard]] bool haveValuesForDimer(DimerPair pair) const;
    [[nodiscard]] const Components& valuesForDimer(DimerPair pair) const;

    [[nodiscard]] inline const auto& values() const { return m_interactions; }

private:

    DimerInteractionValues m_interactions;
    Components m_emptyComponents{};
};
