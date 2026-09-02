#pragma once
#include <vector>
#include <Windows.h>
#include <istream>
#include <fstream>
#include <iostream>

#include "types.hpp"
#include "utils/bmpio.hpp"
#include "utils/timer.hpp"

using namespace std;

namespace Operator
{
    vector<vector<CoreTypes::coord>> loadcord(const string& filepath = "input/detected_boxes.txt");
    BYTE * makeQuadMask(struct CoreTypes::ImgSize imgSize, const vector<vector<CoreTypes::coord>>& maskingCords);
    BYTE * masking(const BYTE * source, const BYTE * mask, struct CoreTypes::ImgSize imgSize);
}