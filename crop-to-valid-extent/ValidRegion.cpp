#include <iostream>
#include "ValidRegion.h"

template<typename T>
ValidRegion<T>::ValidRegion(): ValidRegion(0) {}

template<typename T>
ValidRegion<T>::ValidRegion(T nodata): Region(-1, -1, -1, -1), nodata{nodata}, line_index{-1} {}

template<typename T>
void ValidRegion<T>::UpdateFromLine(T *array, int len) noexcept {
  int index_leftmost_valid{-1}, index_rightmost_valid{len};
  // find the index of the left most valid cell and the right most valid cell
  for (int i = 0; i != len; ++i) {
    T value = array[i];
    if (ValueIsValid(value)) {
      if (index_leftmost_valid == -1) {
        index_leftmost_valid = i;
      }
      index_rightmost_valid = i;
    }
  }

  ++line_index;
  // update the global row/column index
  if (index_leftmost_valid != -1) {
    //  this line has at least one valid cell

    if (top == -1) {
      top = line_index;
    }
    bottom = line_index;
    if (index_leftmost_valid < left || left == -1) {
      left = index_leftmost_valid;
    }
    if (index_rightmost_valid > right || right == -1) {
      right = index_rightmost_valid;
    }
  }
}

template<typename T>
void ValidRegion<T>::PrintGDALTranslateSrcWin() const {
  if (this->RegionIsValid()) {
    static_cast<const Region *>(this)->PrintGDALTranslateSrcWin();
  } else {
    std::cerr << "Region is invalid: " << "left=" << left << ", " << "right" << right << ", " << "top=" << top << ", "
              << "bottom" << bottom << std::endl;
  }
}

template<typename T>
inline bool ValidRegion<T>::ValueIsValid(T value) const noexcept {
  return value != nodata;
}

template<typename T>
bool ValidRegion<T>::RegionIsValid() const noexcept {
  return top > -1 && bottom > -1 && right > -1 && left > -1;
}
