#pragma once

#include <string>
#include <Windows.h>
#include "types.hpp"
#include <cstdint>
#include <iostream>
#include <fstream>

#include "utils/timer.hpp"

namespace BmpIO {
BYTE* loadfile(const std::string& filename, CoreTypes::ImgSize& imgSize, 
               BITMAPFILEHEADER* bmpFHeader, BITMAPINFOHEADER* bmpIHeader);

void savefile(const std::string& filename, const CoreTypes::ImgSize imgSize, 
              const BITMAPFILEHEADER bmpFHeader, const BITMAPINFOHEADER bmpIHeader, 
              const BYTE* imgData);

inline int cordtoidx(const CoreTypes::ImgSize& imgSize, const CoreTypes::coord& c) {
    int temp = imgSize.height - 1 - c.y;
    return (temp * imgSize.width + c.x) * 3;
}

} // namespace BmpIO