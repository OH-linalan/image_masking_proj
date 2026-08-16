#pragma once
#include <vector>
#include <opencv2/opencv.hpp>

#include "utils/timer.hpp"

using namespace std;
using namespace cv;
namespace Operator
{
struct orbData
{
    vector<KeyPoint> prevKeypoint;
    vector<KeyPoint> nextKeypoint;
    vector<vector<DMatch>> matches;
};
struct cvHomographyResult
{
    Mat H;
    vector<Point2f> prevKeypoints;
    vector<Point2f> nextKeypoints;
    vector<DMatch> acceptMatch;
};
Mat LoadCV(const string filename);
struct orbData cvORB(const Mat& prev, const Mat& next);
struct cvHomographyResult cvHomography(const orbData& data, double ratio);
}