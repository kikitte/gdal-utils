#include <cmath>
#include <iostream>

#include <ogrsf_frmts.h>

#include "roi.h"
#include "mapbox/earcut.hpp"

using namespace std::string_literals;

using IterateGeomHandler = void (*)(OGRGeometry *, void *data);

void IterateGeom(const std::string &vector_file, IterateGeomHandler geom_handler, void *geom_handler_data) {
  std::unique_ptr<GDALDataset> dataset{(GDALDataset *) GDALOpenEx(vector_file.c_str(), GDAL_OF_VECTOR, nullptr, nullptr,
                                                                  nullptr)};
  if (dataset == nullptr) {
    throw std::runtime_error("无法打开文件"s + vector_file);
  }
  if (dataset->GetLayerCount() == 0) {
    throw std::runtime_error("文件："s + vector_file + "没有矢量图层"s);
  }

  for (int i = 0; i != dataset->GetLayerCount(); ++i) {
    auto layer = dataset->GetLayer(i);
    for (auto &feat: layer) {
      geom_handler(feat->GetGeometryRef(), geom_handler_data);
    }
  }
}

void AddPolygonToROI(OGRGeometry *geometry, void *data) {
  auto polygons = (ROI::PolygonArray *) data;

  switch (geometry->getGeometryType()) {
    case wkbPolygon:
    case wkbPolygon25D:
    case wkbPolygonM:
    case wkbPolygonZM: {
      auto polygon = geometry->toPolygon();
      polygons->emplace_back(polygon->clone());
    };
      break;
    default: {}
  }
}

ROI::ROI(const std::string &roi_path) {
  // 读取矢量数据所有多边形
  IterateGeom(roi_path, AddPolygonToROI, &polygon_array_);

  // 读取所有多边形坐标
  for (auto &polygon: polygon_array_) {
    polygon_rings_array_.push_back(ROI::ReadPolygonCoords(polygon.get()));
  }
  // 对多边形进行三角剖分
  for (const auto &polygon_coords: polygon_rings_array_) {
    polygon_triangulation_aray_.push_back(mapbox::earcut<ROI::Triangulation_N>(polygon_coords));
  }
  // 计算每个多边形的三角剖分三角形面积累加
  for (size_t i = 0; i != polygon_triangulation_aray_.size(); ++i) {
    std::vector<Point> polygon_vertices;
    for (const auto &polygon_ring: polygon_rings_array_[i]) {
      polygon_vertices.insert(polygon_vertices.cend(), polygon_ring.cbegin(), polygon_ring.cend());
    }
    polygon_vertices_array_.push_back(polygon_vertices);

    polygon_triangle_area_acc_array_.push_back(PolygonTriangleAreaAccArray::value_type{});
    auto &polygon_acc_area = *(polygon_triangle_area_acc_array_.end() - 1);

    const auto &triangles_indices = polygon_triangulation_aray_[i];
    auto triangles_number = static_cast<size_t>(triangles_indices.size() / 3);
    for (size_t j = 0; j != triangles_number; ++j) {
      const auto &p1 = polygon_vertices[triangles_indices[j * 3]];
      const auto &p2 = polygon_vertices[triangles_indices[j * 3 + 1]];
      const auto &p3 = polygon_vertices[triangles_indices[j * 3 + 2]];
      double triangle_area = CalculateTriangleArea(
          p1[0], p1[1],
          p2[0], p2[1],
          p3[0], p3[1]);
      if (polygon_acc_area.empty()) {
        polygon_acc_area.push_back(triangle_area);
      } else {
        double total_area = triangle_area + (*(polygon_acc_area.end() - 1));
        polygon_acc_area.push_back(total_area);
      }
    }
  }

  // 多边形面积累积数组
  for (const auto &polygon_triangle_area_acc: polygon_triangle_area_acc_array_) {
    double polygon_area = (*(polygon_triangle_area_acc.end() - 1));
    if (polygon_area_acc_array_.empty()) {
      polygon_area_acc_array_.push_back(polygon_area);
    } else {
      polygon_area += (*(polygon_area_acc_array_.end() - 1));
      polygon_area_acc_array_.push_back(polygon_area);
    }
  }
}

std::array<double, 2> ROI::GenRandomPoint() {
  // 随机选择用于生成随机点的多边形
  double target_polygon_area = rand_zero_one_(rand_engine_) * (*(polygon_area_acc_array_.end() - 1));
  int target_polygon_index = ROI::FindLowerOrEqualIndex(polygon_area_acc_array_, target_polygon_area);
  // 随机选择用于生成随机点的三角形
  double target_triangle_area =
      rand_zero_one_(rand_engine_) * (*(polygon_triangle_area_acc_array_[target_polygon_index].end() - 1));
  int target_triangle_index =
      ROI::FindLowerOrEqualIndex(polygon_triangle_area_acc_array_[target_polygon_index], target_triangle_area);
  // 依据三角形重心坐标原理生成三角形内随机点
  auto polygon_triangulation = polygon_triangulation_aray_[target_polygon_index];
  double u = rand_zero_one_(rand_engine_), v = rand_zero_one_(rand_engine_);
  ROI::Point tri_p1 = polygon_vertices_array_[target_polygon_index][polygon_triangulation[target_triangle_index * 3]];
  ROI::Point
      tri_p2 = polygon_vertices_array_[target_polygon_index][polygon_triangulation[target_triangle_index * 3 + 1]];
  ROI::Point
      tri_p3 = polygon_vertices_array_[target_polygon_index][polygon_triangulation[target_triangle_index * 3 + 2]];

  if (u + v > 1) {
    u = 1 - u;
    v = 1 - v;
  }

  return {
      u * (tri_p3[0] - tri_p1[0]) + v * (tri_p2[0] - tri_p1[0]) + tri_p1[0],
      u * (tri_p3[1] - tri_p1[1]) + v * (tri_p2[1] - tri_p1[1]) + tri_p1[1],
  };
}

ROI::Rings ROI::ReadPolygonCoords(OGRPolygon *polygon) {
  ROI::Rings coords;

  // exterior ring
  coords.push_back(ROI::Rings::value_type{});
  auto exterior_ring = polygon->getExteriorRing();
  for (int i = 0; i != exterior_ring->getNumPoints(); ++i) {
    coords[0].push_back(ROI::Point{exterior_ring->getX(i), exterior_ring->getY(i)});
  }
  // interior rings
  for (int i = 0; i != polygon->getNumInteriorRings(); ++i) {
    auto interior_ring = polygon->getInteriorRing(i);
    coords.push_back(ROI::Rings::value_type{});
    for (int j = 0; j != interior_ring->getNumPoints(); ++j) {
      coords[coords.size() - 1].push_back(ROI::Point{interior_ring->getX(j), interior_ring->getY(j)});
    }
  }

  return coords;
}

int ROI::FindLowerOrEqualIndex(const std::vector<double> &arr, double target) {
  int low = 0, high = static_cast<int>(arr.size() - 1);
  while (low <= high) {
    int mid = static_cast<int>((low + high) / 2);
    if (target < arr[mid])
      high = mid - 1;
    else if (target > arr[mid])
      low = mid + 1;
    else
      return mid;
  }
  return high + 1;
}

double CalculateTriangleArea(double x1, double y1, double x2, double y2, double x3, double y3) {
  // Heron's formula
  double a = sqrt(pow((x1 - x2), 2) + pow((y1 - y2), 2));
  double b = sqrt(pow((x1 - x3), 2) + pow((y1 - y3), 2));
  double c = sqrt(pow((x2 - x3), 2) + pow((y2 - y3), 2));
  double s = (a + b + c) / 2;
  return sqrt(s * (s - a) * (s - b) * (s - c));
}
