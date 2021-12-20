#pragma once

#include <cstddef>
#include <vector>
#include "Region.h"

template<typename T>
class ValidRegion : public Region {
 public:
  ValidRegion();
  explicit ValidRegion(T);
  void UpdateFromLine(T *, int) noexcept;
  void PrintGDALTranslateSrcWin() const;

 private:
  const T nodata;
  int line_index{};

  [[nodiscard]]
  inline bool ValueIsValid(T) const noexcept;
  [[nodiscard]] bool RegionIsValid() const noexcept;
};
