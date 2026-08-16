#pragma once
#include <string>
#include <vector>
#include <fstream>

#include "math/matrix.hpp"
#include "types.hpp"
#include "utils/timer.hpp"

using namespace std;
namespace Operator
{
struct calib
{
    Math::matrix * K;
    Math::matrix * R;
    Math::matrix * T;
};
struct calib Loadcalib(const string filename);
Math::matrix findH(calib c, calib c2);
vector<vector<CoreTypes::coord>> homography(Math::matrix H, const vector<vector<CoreTypes::coord>>& c);
}