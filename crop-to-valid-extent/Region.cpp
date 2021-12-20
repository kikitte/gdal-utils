#include "Region.h"

#include <limits>
#include  <iostream>

Region::Region(int top, int bottom, int left, int right) : top{top}, bottom{bottom}, left{left}, right{right} {}

void Region::PrintGDALTranslateSrcWin() const {
  std::cout << left << " " << top << " " << (right - left + 1) << " " << (bottom - top + 1) << std::endl;
}

void Region::Union(int &a_top, int &a_bottom, int &a_left, int &a_right) const {
  if (top < a_top) {
    a_top = top;
  }
  if (bottom > a_bottom) {
    a_bottom = bottom;
  }
  if (right > a_right) {
    a_right = right;
  }
  if (left < a_left) {
    a_left = left;
  }
}

void Region::Intersect(int &a_top, int &a_bottom, int &a_left, int &a_right) const {
  if (top > a_top) {
    a_top = top;
  }
  if (bottom < a_bottom) {
    a_bottom = bottom;
  }
  if (left > a_left) {
    a_left = left;
  }
  if (right < a_right) {
    a_right = right;
  }
}
Region::Region() = default;

Region UnionRegions(const std::vector<Region> &regions) {
  int kIntLowest = std::numeric_limits<int>::lowest(), kIntMax = std::numeric_limits<int>::max();

  int top{kIntMax}, bottom{kIntLowest}, left{kIntMax}, right{kIntLowest};
  for (const auto &region: regions) {
    region.Union(top, bottom, left, right);
  }

  return {top, bottom, left, right};
}

Region IntersectRegions(const std::vector<Region> &regions) {
  int kIntLowest = std::numeric_limits<int>::lowest(), kIntMax = std::numeric_limits<int>::max();

  int top{kIntLowest}, bottom{kIntMax}, left{kIntLowest}, right{kIntMax};
  for (const auto &region: regions) {
    region.Intersect(top, bottom, left, right);
  }

  return {top, bottom, left, right};
}
