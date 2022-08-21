#pragma once

#include <vector>
#include <memory>
#include <array>
#include <random>
#include <cstdint>

#include "ogr_geometry.h"

/**
 * 计算区域，可包含多个，使用多个Polygon表示
 */
class ROI {
 public:
  using PolygonArray = std::vector<std::unique_ptr<OGRPolygon>>;

  using Point = std::array<double, 2>;
  using Ring = std::vector<Point>;
  // 多边形坐标数组，包括外环和内环坐标
  using Rings = std::vector<Ring>;
  // 多个多边形坐标数组
  using RingsArray = std::vector<Rings>;
  // 多个多边形三角剖分结果
  using Triangulation_N = uint32_t;
  using PolygonTriangulationAray = std::vector<std::vector<Triangulation_N>>;
  // 每个多边形的三角形(多个)面积累加数组
  using PolygonTriangleAreaAccArray = std::vector<std::vector<double>>;

  explicit ROI(const std::string &roi_path);

  /**
   * 在ROI内随机生成点
   */
  std::array<double, 2> GenRandomPoint();

  [[nodiscard]]  static Rings ReadPolygonCoords(OGRPolygon *);

 private:
  PolygonArray polygon_array_;
  RingsArray polygon_rings_array_;
  PolygonTriangulationAray polygon_triangulation_aray_;
  PolygonTriangleAreaAccArray polygon_triangle_area_acc_array_;
  std::vector<std::vector<Point>> polygon_vertices_array_;
  // 多个多边形扁平化后的顶点坐标数组
  std::vector<double> polygon_area_acc_array_;

  // random engine
  std::mt19937 rand_engine_{std::random_device{}()};
  // random distribution
  std::uniform_real_distribution<double> rand_zero_one_{0.0, 1.0};
  std::uniform_int_distribution<int> random_polygon_index_;

  /**
   * 查找数组当中值等于或第一个小于指定值(target)的项索引
   * 使用二分查找法实现
   */
  [[nodiscard]]  static int FindLowerOrEqualIndex(const std::vector<double> &arr, double target);
};

/**
 * 计算三角形面积
 */
[[nodiscard]] double CalculateTriangleArea(double x1, double y1, double x2, double y2, double x3, double y3);
