#pragma once
#include <QDataStream>

template <typename Container>
inline QDataStream &write_stl_container(QDataStream &ds,
                                        const Container &container) {
  ds << static_cast<int>(container.size());
  for (const auto &val : container) {
    ds << val;
  }
  return ds;
}

template <typename Container>
inline QDataStream &read_stl_container(QDataStream &ds, Container &container) {
  int count = 0;
  ds >> count;
  container.clear();
  container.reserve(count);
  for (int i = 0; i < count; ++i) {
    typename Container::value_type x;
    ds >> x;
    container.push_back(x);
  }
  return ds;
}
