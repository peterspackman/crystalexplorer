#pragma once
#include <occ/3rdparty/nanoflann.hpp>
#include <occ/core/linear_algebra.h>
#include <vector>
#include <memory>

namespace occ::core {

template <typename NumericType>
using KDTree = nanoflann::KDTreeEigenMatrixAdaptor<
    Eigen::Matrix<NumericType, 3, Eigen::Dynamic>, 3, nanoflann::metric_L2,
    false>;

inline constexpr size_t max_leaf = 10;

template <typename NumericType> class DynamicKDTree {
private:
  using VectorType = Eigen::Vector3<NumericType>;

  struct PointCloud {
    std::vector<VectorType> pts;

    inline size_t kdtree_get_point_count() const { return pts.size(); }

    inline NumericType kdtree_get_pt(const size_t idx, const size_t dim) const {
      return pts[idx][dim];
    }

    template <class BBOX> bool kdtree_get_bbox(BBOX &) const { return false; }
  };

  PointCloud cloud;
  using KDTreeType = nanoflann::KDTreeSingleIndexDynamicAdaptor<
      nanoflann::L2_Simple_Adaptor<NumericType, PointCloud>, PointCloud, 3>;

  std::unique_ptr<KDTreeType> index;

public:
  DynamicKDTree(size_t max_leaf = 10, size_t initial_max_points = 1000000)
      : index(std::make_unique<KDTreeType>(
            3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(max_leaf),
            initial_max_points)) {}

  void addPoint(const VectorType &point) {
    size_t index_before = cloud.pts.size();
    cloud.pts.push_back(point);
    index->addPoints(index_before, index_before);
  }

  std::pair<size_t, NumericType> nearest(const VectorType &query) const {
    size_t ret_index;
    NumericType out_dist_sqr;
    nanoflann::KNNResultSet<NumericType> resultSet(1);
    resultSet.init(&ret_index, &out_dist_sqr);
    index->findNeighbors(resultSet, query.data(), nanoflann::SearchParams(10));
    return {ret_index, out_dist_sqr};
  }

  size_t size() const { return cloud.pts.size(); }
};

using KdResultSet = std::vector<std::pair<Eigen::Index, double>>;
using KdRadiusResultSet = nanoflann::RadiusResultSet<double, Eigen::Index>;

} // namespace occ::core
