#include <iostream>
#include <gdal.h>
#include <gdal_priv.h>
#include <cpl_port.h>
#include <cstdlib>
#include <string>
#include "CLI11.hpp"

struct GridLineRecord {
  int top;
  int bottom;
  int left;
  int right;

  GridLineRecord Union(GridLineRecord another) {
    return {
        .top = MIN(top, another.top),
        .bottom = MAX(bottom, another.bottom),
        .left = MIN(left, another.left),
        .right = MAX(right, another.right)};
  }

  void Print() const {
    std::cout << left << " " << top << " " << right - left + 1 << " " << bottom - top + 1;
  }

  [[nodiscard]] bool IsValid() const {
    return bottom >= 0 && top <= bottom && left >= 0 && right >= left;
  }
};

struct LineCache {
  double nodata;

  LineCache(GDALRasterBand *band, int col_buf_num, int row_buf_num)
      : kColBufColNum(col_buf_num), kRowBufRowNum(row_buf_num) {
    this->band = band;
    rows = band->GetYSize();
    cols = band->GetXSize();
    cur_col_group = -1;
    cur_row_group = -1;
    nodata = band->GetNoDataValue();
    rows_buf = nullptr;
    cols_buf = nullptr;
    cols_buf_transpose = nullptr;
  }

  ~LineCache() {
    FreeColsBuf();
    FreeRowsBuf();
  }

  void AllocateRowsBuf() {
    if (rows_buf == nullptr) {
      rows_buf = new double[(long) cols * (long) kRowBufRowNum];
    }
  }

  void FreeRowsBuf() {
    delete[] rows_buf;
    rows_buf = nullptr;
  }

  double *ReadRow(int row_index, CPLErr &read_err) {
    int row_group = row_index / kRowBufRowNum;
    if (row_group != cur_row_group) {
      int row_begin = row_group * kRowBufRowNum;
      int row_num = (row_begin + kRowBufRowNum < rows) ? kRowBufRowNum : rows - row_begin;
      read_err = band->RasterIO(GF_Read,
                                0,
                                row_begin,
                                cols,
                                row_num,
                                rows_buf,
                                cols,
                                row_num,
                                GDT_Float64,
                                0,
                                0,
                                nullptr);
      cur_row_group = row_group;
    }
    return rows_buf + cols * (row_index - kRowBufRowNum * cur_row_group);
  }

  void AllocateColsBuf() {
    if (cols_buf == nullptr) {
      cols_buf = new double[(long) rows * (long) kColBufColNum];
    }
    if (cols_buf_transpose == nullptr) {
      cols_buf_transpose = new double[rows * kColBufColNum];
    }
  }

  void FreeColsBuf() {
    delete[] cols_buf;
    delete[] cols_buf_transpose;
    cols_buf = nullptr;
    cols_buf_transpose = nullptr;
  }
  double *ReadCol(int col_index, CPLErr &read_err) {
    int col_group = col_index / kColBufColNum;
    if (col_group != cur_col_group) {
      int col_begin = col_group * kColBufColNum;
      int col_num = (col_begin + kColBufColNum < cols) ? kColBufColNum : cols - col_begin;
      read_err = band->RasterIO(GF_Read,
                                col_begin,
                                0,
                                col_num,
                                rows,
                                cols_buf,
                                col_num,
                                rows,
                                GDT_Float64,
                                0,
                                0,
                                nullptr);
      cur_col_group = col_group;

      // 将RasterIO读取的列分组转置使得每一列的数据的内存连续
      double *transpose_mem = cols_buf_transpose;
      for (int i = 0; i < col_num; ++i) {
        for (int j = 0; j < rows; ++j) {
          *transpose_mem++ = cols_buf[col_num * j + i];
        }
      }
      memcpy(cols_buf, cols_buf_transpose, rows * col_num * sizeof(double));
    }
    return cols_buf + rows * (col_index - kColBufColNum * cur_col_group);
  }

 private:
  GDALRasterBand *band;
  double *rows_buf = nullptr;
  double *cols_buf = nullptr;
  double *cols_buf_transpose = nullptr;
  int rows;
  int cols;
  int cur_row_group;
  int cur_col_group;
  const int kRowBufRowNum;
  const int kColBufColNum;

};

bool DetectMinimumExtent(GDALRasterBand *band, GridLineRecord &line_record, int col_buf_num, int row_buf_num);

bool CheckGridLineIsValid(LineCache &line_cache, bool is_row, int line_index, int line_size, bool &has_error);

int main(int argc, char **argv) {
  CLI::App
      app("Print the minimum valid extent of raster which can be used as -srcwin parameter in gdal_translate tool");
  std::string input_raster;
  app.add_option("--raster", input_raster, "")->required()->check(CLI::ExistingFile);
  int col_buf_num, row_buf_num;
  app.add_option("--col_buffer_size",
                 col_buf_num,
                 "The columns number that every time reads from dist to memory.")->required();
  app.add_option("--row_buffer_size",
                 row_buf_num,
                 "The rows number that every time reads from dist to memory.")->required();
  CLI11_PARSE(app, argc, argv)

  GDALAllRegister();
  auto in_ds = (GDALDataset *) GDALOpen(input_raster.c_str(), GA_ReadOnly);
  if (in_ds == nullptr) {
    std::cerr << "Failed to open file: " << input_raster << std::endl;
    exit(EXIT_FAILURE);
  }
  GridLineRecord final_record{.top=INT32_MAX, .bottom=INT32_MIN, .left=INT32_MAX, .right=INT32_MIN};
  int in_ds_band_number = in_ds->GetRasterCount();
  for (int i = 1; i <= in_ds_band_number; ++i) {
    GridLineRecord record{};
    bool has_error = DetectMinimumExtent(in_ds->GetRasterBand(i), record, col_buf_num, row_buf_num);
    if (has_error) {
      std::cerr << "Failed to read raster data." << std::endl;
      GDALClose(in_ds);
      exit(EXIT_FAILURE);
    }
    final_record = final_record.Union(record);
  }

  if (!final_record.IsValid()) {
    std::cerr << "There is no valid cells on your input raster, Please check it." << std::endl;
    GDALClose(in_ds);
    exit(EXIT_FAILURE);
  }

  final_record.Print();

  return 0;
}

bool DetectMinimumExtent(GDALRasterBand *band, GridLineRecord &line_record, int col_buf_num, int row_buf_num) {
  bool has_error;
  int col_number = band->GetXSize();
  int row_number = band->GetYSize();

  LineCache line_cache(band, col_buf_num, row_buf_num);

  // init line record
  line_record.top = 0;
  line_record.left = 0;
  line_record.bottom = row_number - 1;
  line_record.right = col_number - 1;

  // find the first row/col from exterior to interior that has at least one valid cell
  line_cache.AllocateRowsBuf();
  for (int i = 0; i < row_number; ++i) {
    // from top to bottom
    bool row_is_valid = CheckGridLineIsValid(line_cache, true, i, col_number, has_error);
    if (has_error) {
      goto clean;
    }
    if (row_is_valid) {
      break;
    } else {
      ++line_record.top;
    }
  }
  for (int i = row_number - 1; i >= 0; --i) {
    // from bottom to top
    bool row_is_valid = CheckGridLineIsValid(line_cache, true, i, col_number, has_error);
    if (has_error) {
      goto clean;
    }
    if (row_is_valid) {
      break;
    } else {
      --line_record.bottom;
    }
  }
  line_cache.FreeRowsBuf();
  line_cache.AllocateColsBuf();
  for (int i = 0; i < col_number; ++i) {
    // from left to right
    bool col_is_valid = CheckGridLineIsValid(line_cache, false, i, row_number, has_error);
    if (has_error) {
      goto clean;
    }
    if (col_is_valid) {
      break;
    } else {
      ++line_record.left;
    }
  }
  for (int i = col_number - 1; i >= 0; --i) {
    // from right to left
    bool col_is_valid = CheckGridLineIsValid(line_cache, false, i, row_number, has_error);
    if (has_error) {
      goto clean;
    }
    if (col_is_valid) {
      break;
    } else {
      --line_record.right;
    }
  }
  line_cache.FreeColsBuf();

  clean:
  line_cache.FreeRowsBuf();
  line_cache.FreeColsBuf();
  return has_error;
}

bool CheckGridLineIsValid(LineCache &line_cache,
                          bool is_row,
                          int line_index,
                          int line_size,
                          bool &has_error) {
  has_error = false;

  CPLErr read_err = CE_None;
  double *line_buf = is_row ? line_cache.ReadRow(line_index, read_err) : line_cache.ReadCol(line_index, read_err);
  if (read_err != CE_None) {
    has_error = true;
    return false;
  }

  for (int i = 0; i < line_size; ++i) {
    if (line_buf[i] != line_cache.nodata) {
      has_error = false;
      return true;
    }
  }

  return false;
}