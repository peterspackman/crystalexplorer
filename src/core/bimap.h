#pragma once
#include <ankerl/unordered_dense.h>
#include <vector>

template <typename T, typename IndexT = size_t> class BiMap {
public:
  using index_type = IndexT;
  using value_type = T;
  using map_type = ankerl::unordered_dense::map<T, index_type>;
  using vector_type = std::vector<T>;

  index_type add(const T &object) {
    if (auto it = m_objectToIndex.find(object); it != m_objectToIndex.end()) {
      return it->second;
    }

    index_type index = static_cast<index_type>(m_indexToObject.size());
    m_indexToObject.push_back(object);
    m_objectToIndex[object] = index;
    return index;
  }

  index_type add(T &&object) {
    if (auto it = m_objectToIndex.find(object); it != m_objectToIndex.end()) {
      return it->second;
    }

    index_type index = static_cast<index_type>(m_indexToObject.size());
    m_indexToObject.push_back(std::move(object));
    m_objectToIndex[m_indexToObject.back()] = index;
    return index;
  }

  const T get(index_type index) const {
    auto idx = static_cast<typename vector_type::size_type>(index);
    if (idx >= m_indexToObject.size())
      return nullptr;
    return m_indexToObject[idx];
  }

  T get(index_type index) {
    auto idx = static_cast<typename vector_type::size_type>(index);
    if (idx >= m_indexToObject.size())
      return nullptr;
    return m_indexToObject[idx];
  }

  std::optional<index_type> getIndex(const T &object) const {
    auto it = m_objectToIndex.find(object);
    if (it == m_objectToIndex.end())
      return std::nullopt;
    return it->second;
  }

  bool remove(const T &object) {
    auto it = m_objectToIndex.find(object);
    if (it == m_objectToIndex.end())
      return false;

    auto indexToRemove =
        static_cast<typename vector_type::size_type>(it->second);
    auto lastIndex = m_indexToObject.size() - 1;

    if (indexToRemove != lastIndex) {
      // Move last element to the gap
      m_indexToObject[indexToRemove] = std::move(m_indexToObject[lastIndex]);
      m_objectToIndex[m_indexToObject[indexToRemove]] =
          static_cast<index_type>(indexToRemove);
    }

    m_indexToObject.pop_back();
    m_objectToIndex.erase(it);
    return true;
  }

  void clear() {
    m_indexToObject.clear();
    m_objectToIndex.clear();
  }

  auto size() const { return m_indexToObject.size(); }

  bool empty() const { return m_indexToObject.empty(); }

  // Iterator support
  auto begin() { return m_indexToObject.begin(); }
  auto end() { return m_indexToObject.end(); }
  auto begin() const { return m_indexToObject.begin(); }
  auto end() const { return m_indexToObject.end(); }
  auto cbegin() const { return m_indexToObject.cbegin(); }
  auto cend() const { return m_indexToObject.cend(); }

  // Access to underlying containers if needed
  const vector_type &objects() const { return m_indexToObject; }
  const map_type &indices() const { return m_objectToIndex; }

private:
  vector_type m_indexToObject;
  map_type m_objectToIndex;
};
