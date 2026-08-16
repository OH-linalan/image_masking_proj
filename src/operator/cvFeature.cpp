#include "operator/cvFeature.hpp"

namespace Operator
{
Mat LoadCV(const string filename)
{
    timer t("cvFeature: LoadCV method : " + filename);
    Mat img = imread(filename);
    if (img.empty())
    {
        std::cout << "Can't find image: " << filename << std::endl;
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
    //특징점 개수 1000개 제한
    static Ptr<ORB> detector = ORB::create(1000);
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

    return orbData{std::move(prevKeypoints), std::move(nextKeypoints), std::move(matches)};
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
        prevKeypoints.push_back(data.prevKeypoint[m.queryIdx].pt);
        nextKeypoints.push_back(data.nextKeypoint[m.trainIdx].pt);
    }

    Mat H;
    if (prevKeypoints.size() >= 4)
    {
        timer t("cvFeature: Find Homography with RANSAC");
        H = findHomography(prevKeypoints, nextKeypoints, RANSAC);
    }
    else
    {
        std::cout << "[WARN] Not enough matches for Homography: " << prevKeypoints.size() << std::endl;
        H = Mat::eye(3, 3, CV_64FC1);
    }

    return cvHomographyResult{H, prevKeypoints, nextKeypoints, acceptMatch};
}
}