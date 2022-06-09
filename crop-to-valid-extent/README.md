Print the smallest valid extent of a raster(ignores the blank rows/cols).

### Build

```bash
cd crop-to-valid-extent
cmake -D CMAKE_BUILD_TYPE=Release -S . -B build
cmake --build ./build
# The executable file is crop-to-valid-extent
```

### Usage

```bash
in_raster=example.tif
out_raster=example_cropped.tif
gdal_translate -srcwin $(crop-to-valid-extent $in_raster) $in_raster $out_raster
```





