#pragma once
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <cmath>

#include "types.hpp"
#include "utils/timer.hpp"

using namespace std;
using namespace cv;

namespace Spec
{
inline double rmse(Mat img1, Mat img2)
{
    timer t("spec: rmse method");
    if (img1.empty() || img2.empty()) return 999.0;
    if (img1.size() != img2.size()) {
        resize(img1, img1, img2.size());
    }

    Mat diff;
    absdiff(img1, img2, diff);
    diff.convertTo(diff, CV_32F);
    Mat sq = diff.mul(diff);
    Scalar sum = cv::sum(sq);
    double total = sum[0] + sum[1] + sum[2];
    double elem = img1.rows * img1.cols * img1.channels();
    return sqrt(total / elem);
}

inline double iou(const vector<vector<CoreTypes::coord>>& c1, const vector<vector<CoreTypes::coord>>& c2)
{
    timer k("spec: iou method");
    size_t num_objects = min(c1.size(), c2.size());
    if (num_objects == 0) return 0.0;
    double total_iou = 0.0;
    int valid_count = 0;

    for (size_t obj = 0; obj < num_objects; obj++)
    {
        if (c1[obj].size() < 4 || c2[obj].size() < 4) continue;

        double c1_xmin = min({(double)c1[obj][0].x, (double)c1[obj][1].x, (double)c1[obj][2].x, (double)c1[obj][3].x});
        double c1_xmax = max({(double)c1[obj][0].x, (double)c1[obj][1].x, (double)c1[obj][2].x, (double)c1[obj][3].x});
        double c1_ymin = min({(double)c1[obj][0].y, (double)c1[obj][1].y, (double)c1[obj][2].y, (double)c1[obj][3].y});
        double c1_ymax = max({(double)c1[obj][0].y, (double)c1[obj][1].y, (double)c1[obj][2].y, (double)c1[obj][3].y});

        double c2_xmin = min({(double)c2[obj][0].x, (double)c2[obj][1].x, (double)c2[obj][2].x, (double)c2[obj][3].x});
        double c2_xmax = max({(double)c2[obj][0].x, (double)c2[obj][1].x, (double)c2[obj][2].x, (double)c2[obj][3].x});
        double c2_ymin = min({(double)c2[obj][0].y, (double)c2[obj][1].y, (double)c2[obj][2].y, (double)c2[obj][3].y});
        double c2_ymax = max({(double)c2[obj][0].y, (double)c2[obj][1].y, (double)c2[obj][2].y, (double)c2[obj][3].y});
        Rect2d c1_r(c1_xmin, c1_ymin, max(0.0, c1_xmax - c1_xmin), max(0.0, c1_ymax - c1_ymin));
        Rect2d c2_r(c2_xmin, c2_ymin, max(0.0, c2_xmax - c2_xmin), max(0.0, c2_ymax - c2_ymin));

        Rect2d inter = c1_r & c2_r;
        double interArea = inter.area();
        double unionArea = c1_r.area() + c2_r.area() - interArea;

        if (unionArea > 1e-6) {
            total_iou += (interArea / unionArea);
            valid_count++;
        }
    }

    if (valid_count == 0) return 0.0;
    return total_iou / valid_count;
}
}