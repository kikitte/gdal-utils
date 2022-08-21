#include <iostream>
#include <cstdlib>

#include <gdal.h>
#include <ogrsf_frmts.h>

#include "../source/roi.h"

int main(int argc, char **argv) {
  GDALAllRegister();

  const char *polygon_path = argv[1];
  const char *output_random_points = argv[2];
  const int random_points_num = atoi(argv[3]);

  ROI roi(polygon_path);

  auto driver = GetGDALDriverManager()->GetDriverByName("GPKG");
  GDALDriver::QuietDelete(output_random_points);
  auto dataset = driver->Create(output_random_points, 0, 0, 0, GDT_Unknown, nullptr);
  if (dataset == nullptr) {
    std::cout << "创建数据集失败" << std::endl;
    return 1;
  }

  auto layer = dataset->CreateLayer("points", nullptr, wkbPoint, nullptr);
  if (layer == nullptr) {
    std::cout << "创建图层失败" << std::endl;
    return 1;
  }

  for (int i = 0; i != random_points_num; ++i) {
    OGRFeature feat{layer->GetLayerDefn()};

    auto point_xy = roi.GenRandomPoint();
    OGRPoint pt{point_xy[0], point_xy[1]};
    feat.SetGeometry(&pt);
    auto err = layer->CreateFeature(&feat);
    if (err != CE_None) {
      std::cout << "创建要素失败" << std::endl;
      return 1;
    }
  }

  GDALClose(dataset);

  return 0;
}
