Read the smallest valid extent of a raster, so it ignores the blank rows/cols.

1. mian.cpp - use Float64 as gdal data type, so it is suitable for most raster but with more memory  consumption.
2. main_byte.cpp - use byte as gdal data type.

### Usage

```bash
#!/bin/bash

current_dir=$(dirname $0)
input=$1
output=$2

gdal_translate -srcwin $(crop_to_valid_extent --raster $input --col_buffer_size 10000 --row_buffer_size 1000) $input $output
```

### TODO

Using generic type  for raster that in different data type.



