#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <gdal_priv.h>
#include "deps/CLI11.hpp"
#include "ValidRegion.h"
#include "Region.h"

Region GetBandValidRegion(GDALRasterBand *);
template<typename T>
Region GetTypedBandValidRegion(GDALRasterBand *);

int main(int argc, char **argv) {
  CLI::App app
      ("Print the minimum valid extent of raster band(s) which can be used as -srcwin parameter in gdal_translate cmd tool");

  std::string input_raster;
  int band_index{0};
  bool union_region;

  app.add_option("--raster", input_raster, "")->required()->check(CLI::ExistingFile);
  app.add_option("--band",
                 band_index,
                 "specific which band will be used to extract the valid extent, zero means all bands.")->default_val(0);
  app.add_flag("--union", union_region, "Print the union of regions. Default is intersection.")->default_val(false);
  CLI11_PARSE(app, argc, argv);

  GDALAllRegister();

  auto in_ds = (GDALDataset *) GDALOpen(input_raster.c_str(), GA_ReadOnly);
  if (in_ds == nullptr) {
    std::cerr << "Failed to open file: " << input_raster << std::endl;
    exit(EXIT_FAILURE);
  }

  int band_number = in_ds->GetRasterCount();
  if (band_number == 0 || band_index < 0 || band_index > band_number) {
    std::cerr << "Invalid Band: " << band_index << std::endl;
    exit(EXIT_FAILURE);
  }

  if (band_index != 0) {
    auto band = in_ds->GetRasterBand(band_index);
    auto band_valid_region = GetBandValidRegion(band);
    band_valid_region.PrintGDALTranslateSrcWin();
  } else {
    std::vector<Region> regions;
    for (int i = 1; i <= band_number; ++i) {
      auto band = in_ds->GetRasterBand(i);
      auto band_valid_region = GetBandValidRegion(band);
      regions.push_back(band_valid_region);
    }
    Region region;
    if (union_region) {
      region = UnionRegions(regions);
    } else {
      region = IntersectRegions(regions);
    }
    region.PrintGDALTranslateSrcWin();
  }

  return 0;
}

Region GetBandValidRegion(GDALRasterBand *band) {
  GDALDataType data_type = band->GetRasterDataType();
  switch (data_type) {
    case GDT_Byte:return GetTypedBandValidRegion<char>(band);
    case GDT_Int16:return GetTypedBandValidRegion<int16_t>(band);
    case GDT_UInt16:return GetTypedBandValidRegion<uint16_t>(band);
    case GDT_Int32:return GetTypedBandValidRegion<int32_t>(band);
    case GDT_UInt32:return GetTypedBandValidRegion<uint32_t>(band);
    case GDT_Float32:return GetTypedBandValidRegion<float_t>(band);
    case GDT_Float64:return GetTypedBandValidRegion<double_t>(band);
    default: return {-1, -1, -1, -1};
  }
};

template<typename T>
Region GetTypedBandValidRegion(GDALRasterBand *band) {
  int cols = band->GetXSize(), rows = band->GetYSize();
  std::unique_ptr<T[]> line_data{new T[cols]};
  ValidRegion<T> region(static_cast<T>(band->GetNoDataValue()));

  for (int row = 0; row != rows; ++row) {
    auto err = band->RasterIO(GF_Read, 0, row, cols, 1, line_data.get(), cols, 1, band->GetRasterDataType(), 0, 0);
    if (err != CE_None) {
      throw std::runtime_error("Raster IO Error!");
    }
    region.UpdateFromLine(line_data.get(), cols);
  }

  return region;
}
