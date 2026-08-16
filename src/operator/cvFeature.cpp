#include "operator/cvFeature.hpp"
#include <iostream>
#include <algorithm>

namespace Operator
{
Mat LoadCV(const string filename)
{
    timer t("cvFeature: LoadCV method : " + filename);
    Mat img = imread(filename);
    if (img.empty())
    {
        cout << "Can't find image: " << filename << endl;
        return Mat();
    }
    return img;
}

struct orbData cvORB(const Mat& prev, const Mat& next)
{
    timer k("cvFeature: cvORB method");
    vector<KeyPoint> prevKeypoints, nextKeypoints;
    Mat prevDesc, nextDesc;
    vector<vector<DMatch>> matches;
    //특징점 개수 500개 제한
    static Ptr<ORB> detector = ORB::create(500);
    static Ptr<BFMatcher> matcher = BFMatcher::create(NORM_HAMMING);
    {
        timer t("cvFeature: FAST & BRIEF (prev)");
        detector->detectAndCompute(prev, noArray(), prevKeypoints, prevDesc);
    }
    {
        timer t("cvFeature: FAST & BRIEF (next)");
        detector->detectAndCompute(next, noArray(), nextKeypoints, nextDesc);
    }
    if (!prevDesc.empty() && !nextDesc.empty() && prevDesc.rows >= 2 && nextDesc.rows >= 2)
    {
        if (prevDesc.type() != CV_8U) prevDesc.convertTo(prevDesc, CV_8U);
        if (nextDesc.type() != CV_8U) nextDesc.convertTo(nextDesc, CV_8U);

        timer t("cvFeature: cvORB method(knnMatch operation)");
        matcher->clear();
        matcher->knnMatch(prevDesc, nextDesc, matches, 2);
    }

    return orbData{move(prevKeypoints), move(nextKeypoints), move(matches)};
}

struct orbData LocalORB(const Mat& prev, const Mat& next, const vector<vector<CoreTypes::coord>>& maskingCords)
{
    timer k("cvFeature: LocalORB method");
    vector<KeyPoint> prevKeypoints, nextKeypoints;
    Mat prevDesc, nextDesc;
    vector<vector<DMatch>> matches;

    static Ptr<ORB> detector = ORB::create(500);
    static Ptr<BFMatcher> matcher = BFMatcher::create(NORM_HAMMING);

    Mat prevGray, nextGray;
    if (prev.channels() == 3) cvtColor(prev, prevGray, COLOR_BGR2GRAY);
    else prevGray = prev;

    if (next.channels() == 3) cvtColor(next, nextGray, COLOR_BGR2GRAY);
    else nextGray = next;

    int min_x = prev.cols, min_y = prev.rows, max_x = 0, max_y = 0;
    bool has_cords = false;

    for (const auto& box : maskingCords) {
        for (const auto& pt : box) {
            min_x = (std::min)(min_x, pt.x);
            min_y = (std::min)(min_y, pt.y);
            max_x = (std::max)(max_x, pt.x);
            max_y = (std::max)(max_y, pt.y);
            has_cords = true;
        }
    }

    Rect prevRoi(0, 0, prev.cols, prev.rows);
    Rect nextRoi(0, 0, next.cols, next.rows);

    if (has_cords && min_x < max_x && min_y < max_y) {
        int x1 = (std::max)(0, min_x);
        int y1 = (std::max)(0, min_y);
        int x2 = (std::min)(prev.cols, max_x);
        int y2 = (std::min)(prev.rows, max_y);
        int w1 = (std::max)(0, x2 - x1);
        int h1 = (std::max)(0, y2 - y1);

        if (w1 > 0 && h1 > 0) {
            prevRoi = Rect(x1, y1, w1, h1);
        }
        int pad = 50;
        int nx1 = (std::max)(0, min_x - pad);
        int ny1 = (std::max)(0, min_y - pad);
        int nx2 = (std::min)(next.cols, max_x + pad);
        int ny2 = (std::min)(next.rows, max_y + pad);
        int nw = (std::max)(0, nx2 - nx1);
        int nh = (std::max)(0, ny2 - ny1);

        if (nw > 0 && nh > 0) {
            nextRoi = Rect(nx1, ny1, nw, nh);
        }
    }

    Mat prevCropped = prevGray(prevRoi);
    Mat nextCropped = nextGray(nextRoi);

    //maskingCords로 마스크 데이터만 다각화
    {
        timer t("cvFeature: FAST & BRIEF (prev - masked)");
        detector->detectAndCompute(prevCropped, noArray(), prevKeypoints, prevDesc);
        for (auto& kp : prevKeypoints) {
            kp.pt.x += prevRoi.x;
            kp.pt.y += prevRoi.y;
        }
    }
    {
        timer t("cvFeature: FAST & BRIEF (next)");
        detector->detectAndCompute(nextCropped, noArray(), nextKeypoints, nextDesc);
        for (auto& kp : nextKeypoints) {
            kp.pt.x += nextRoi.x;
            kp.pt.y += nextRoi.y;
        }
    }
    if (!prevDesc.empty() && !nextDesc.empty() && prevDesc.rows >= 2 && nextDesc.rows >= 2)
    {
        timer t("cvFeature: cvORB method(knnMatch operation)");
        matcher->knnMatch(prevDesc, nextDesc, matches, 2);
    }

    return orbData{move(prevKeypoints), move(nextKeypoints), move(matches)};
} 

struct cvHomographyResult cvHomography(const orbData& data, double ratio)
{
    timer k("cvFeature: cvHomography method");
    vector<DMatch> acceptMatch;
    
    {
        timer t("cvFeature: lowe's ratio test");
        for (const auto& m : data.matches)
        {
            if (m.size() >= 2 && m[0].distance < ratio * m[1].distance)
            {
                acceptMatch.push_back(m[0]);
            }
        }
    }

    vector<Point2f> prevKeypoints, nextKeypoints;
    for (const auto& m : acceptMatch)
    {
        if (m.queryIdx >= 0 && static_cast<size_t>(m.queryIdx) < data.prevKeypoint.size() &&
            m.trainIdx >= 0 && static_cast<size_t>(m.trainIdx) < data.nextKeypoint.size())
        {
            prevKeypoints.push_back(data.prevKeypoint[m.queryIdx].pt);
            nextKeypoints.push_back(data.nextKeypoint[m.trainIdx].pt);
        }
    }

    Mat H;
    if (prevKeypoints.size() >= 4)
    {
        timer t("cvFeature: Find Homography with RANSAC");
        H = findHomography(prevKeypoints, nextKeypoints, RANSAC);
    }
    else
    {
        cout << "[WARN] Not enough matches for Homography: " << prevKeypoints.size() << endl;
        H = Mat::eye(3, 3, CV_64FC1);
    }

    return cvHomographyResult{H, prevKeypoints, nextKeypoints, acceptMatch};
}

Mat createCvMask(const Size& imgSize, const vector<vector<CoreTypes::coord>>& maskingCords)
{
    if (maskingCords.empty()) {
        return Mat();
    }

    Mat mask = Mat::zeros(imgSize, CV_8UC1);

    for (const auto& box : maskingCords) {
        if (box.size() < 4) continue;
        // ul, ur, dr, dl 순서로 다각형 점 구성
        vector<Point> poly = {
            Point(box[0].x, box[0].y), // ul
            Point(box[1].x, box[1].y), // ur
            Point(box[3].x, box[3].y), // dr
            Point(box[2].x, box[2].y)  // dl
        };

        const Point* pts = poly.data();
        int npts = 4;
        fillPoly(mask, &pts, &npts, 1, Scalar(255));
    }

    return mask;
}
}