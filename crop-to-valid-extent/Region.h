#pragma once

#include <vector>

class Region {
 public:
  Region();
  Region(int top, int bottom, int left, int right);
  void PrintGDALTranslateSrcWin() const;
  void Union(int &, int &, int &, int &) const;
  void Intersect(int &, int &, int &, int &) const;

 protected:
  // the first index of line or column which contains at least one valid cell in each direction
  int top{}, bottom{}, left{}, right{};
};

Region UnionRegions(const std::vector<Region> &regions);
Region IntersectRegions(const std::vector<Region> &regions);
