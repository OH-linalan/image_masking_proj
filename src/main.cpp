#include <iostream>
#include <string>
#include <fstream>
#include <Windows.h>
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include <vector>
#include <tuple>
#include <chrono>
#include <filesystem>

#include "types.hpp"
#include "utils/timer.hpp"
#include "utils/bmpio.hpp"
#include "utils/logger.hpp"
#include "math/matrix.hpp"
#include "operator/masking.hpp"
#include "operator/cameraParam.hpp"
#include "operator/cvFeature.hpp"
#include "spec.hpp"

using namespace std;
using namespace cv;
namespace fs = std::filesystem;

Logger g_logger(LogLevel::DEBUG, "output/log.txt");

struct FramePair {
    string imgPath;
    string txtPath;
    string boxPath;
    string stemName;
};

bool compareNatural(const fs::path& a, const fs::path& b) {
    string sa = a.stem().string();
    string sb = b.stem().string();
    if (sa.length() != sb.length()) {
        return sa.length() < sb.length();
    }
    return sa < sb;
}

vector<vector<CoreTypes::coord>> instance(const FramePair& firstFrame, const FramePair& secondFrame, int pairIdx, const vector<vector<CoreTypes::coord>>& currentCoords) {
    struct CoreTypes::ImgSize fimgSize = {0, 0}, simgSize = {0, 0};
    uint8_t *fmaskedData = nullptr, *smaskedData = nullptr, *smaskedDataCV = nullptr;
    uint8_t* final_smaskedData = nullptr;
    Mat comp_result, secondCV;
    vector<vector<CoreTypes::coord>> homographyCords, homographyCordsCV;
    vector<vector<CoreTypes::coord>> nextTrackedCoords;
    string chosen_h_name = "Hcv";
    double diff_threshold = 1e-3;
    double error = 0.0;
    double rmse_val = 0.0;
    double iou_val = 0.0;

    {
        timer t("Total Execution Time (Pair " + to_string(pairIdx) + ")");
        
        //first.bmp 로드
        BITMAPFILEHEADER fbmpFHeader;
        BITMAPINFOHEADER fbmpIHeader;
        auto fimgData = BmpIO::loadfile(firstFrame.imgPath.c_str(), fimgSize, &fbmpFHeader, &fbmpIHeader);

        BITMAPFILEHEADER sbmpFHeader;
        BITMAPINFOHEADER sbmpIHeader;
        auto simgData = BmpIO::loadfile(secondFrame.imgPath.c_str(), simgSize, &sbmpFHeader, &sbmpIHeader);
        
        //마스킹 좌표 설정, 마스크 데이터 생성
        auto maskingCoord = Operator::loadcord(firstFrame.boxPath);
        if (maskingCoord.empty()) {
            maskingCoord = currentCoords;
        }

        auto maskData = Operator::makeQuadMask(fimgSize, maskingCoord);

        //마스킹 수행
        fmaskedData = Operator::masking(fimgData, maskData, fimgSize);

        //호모그래피 계산
        auto fdata = Operator::Loadcalib(firstFrame.txtPath.c_str());
        auto sdata = Operator::Loadcalib(secondFrame.txtPath.c_str());
        auto H = Operator::findH(fdata, sdata);

        //호모그래피 행렬을 마스킹 좌표에 적용
        homographyCords = Operator::homography(H, maskingCoord);

        //변환된 좌표를 이용하여 두 번째 이미지에 마스킹 적용
        auto smaskData = Operator::makeQuadMask(simgSize, homographyCords);
        smaskedData = Operator::masking(simgData, smaskData, simgSize);

        //------------------------OPENCV------------------------
        //OpenCV로 이미지 로드
        auto firstCV = Operator::LoadCV(firstFrame.imgPath.c_str());
        secondCV = Operator::LoadCV(secondFrame.imgPath.c_str());
        //ORB 특징점 검출 및 매칭
        auto orbResult = Operator::LocalORB(firstCV, secondCV, maskingCoord);

        auto HcvResult = Operator::cvHomography(orbResult, 0.75);

        auto Hcv = HcvResult.H;
        //예외처리
        bool hcv_valid = (!Hcv.empty() && Hcv.rows == 3 && Hcv.cols == 3);
        if (!hcv_valid) {
            g_logger.log(LogLevel::WARN, "Hcv calculation failed. Fallback to Identity Matrix.");
            Hcv = Mat::eye(3, 3, CV_64FC1);
        }

        double ** HcvData;
        HcvData = new double*[Hcv.rows];
        for (int i = 0; i < Hcv.rows; i++) {
            HcvData[i] = new double[Hcv.cols];
            for (int j = 0; j < Hcv.cols; j++) {
                HcvData[i][j] = Hcv.at<double>(i, j);
            }
        }
        Math::matrix HcvM(Hcv.rows, Hcv.cols, HcvData);

        //호모그래피 행렬을 마스킹 좌표에 적용
        homographyCordsCV = Operator::homography(HcvM, maskingCoord);

        auto smaskDataCV = Operator::makeQuadMask(simgSize, homographyCordsCV);
        smaskedDataCV = Operator::masking(simgData, smaskDataCV, simgSize);

        //-----------------specification-------------------
        //matrix class H를 OpenCV의 Mat으로 변환
        Mat Hcam = Mat::zeros(fdata.K->row, fdata.K->colm, CV_64FC1);
        {
            Math::matrix H_temp = Operator::findH(fdata, sdata);
            for (int i = 0; i < H_temp.row; i++) {
                for (int j = 0; j < H_temp.colm; j++) {
                    Hcam.at<double>(i, j) = H_temp.data[i][j];
                }
            }
        }
        //first.bmp를 Hcam으로 워핑
        Mat firstWarp_Hcam;
        warpPerspective(firstCV, firstWarp_Hcam, Hcam, firstCV.size());
        //워핑된 이미지의 그레이스케일을 구함
        Mat gray, thresh;
        cvtColor(firstWarp_Hcam, gray, COLOR_BGR2GRAY);
        cv::threshold(gray, thresh, 1, 255, THRESH_BINARY);
        //보정할 영역을 찾아서 crop & resize
        Rect roi_cam = boundingRect(thresh);
        Mat firstWarp_Hcam_resized;
        if (roi_cam.width > 0 && roi_cam.height > 0) {
            Mat cropped = firstWarp_Hcam(roi_cam);
            resize(cropped, firstWarp_Hcam_resized, firstCV.size(), 0, 0, INTER_LINEAR);
        } else {
            firstWarp_Hcam_resized = firstWarp_Hcam.clone();
        }
        //first.bmp를 Hcv로 워핑
        Mat firstWarp_Hcv;
        warpPerspective(firstCV, firstWarp_Hcv, Hcv, firstCV.size());
        //워핑된 이미지의 그레이스케일을 구함
        cvtColor(firstWarp_Hcv, gray, COLOR_BGR2GRAY);
        cv::threshold(gray, thresh, 1, 255, THRESH_BINARY);
        //보정할 영역을 찾아서 crop & resize
        Rect roi_cv = boundingRect(thresh);
        Mat firstWarp_Hcv_resized;
        if (roi_cv.width > 0 && roi_cv.height > 0) {
            Mat cropped = firstWarp_Hcv(roi_cv);
            resize(cropped, firstWarp_Hcv_resized, firstCV.size(), 0, 0, INTER_LINEAR);
        } else {
            firstWarp_Hcv_resized = firstWarp_Hcv.clone();
        }
        //카메라 파라미터 호모그래피의 역행렬을 구하고 전체 호모그래피와 곱하여 객체의 움직임 호모그래피를 분리함
        Mat Hcam_inv;
        error = 1.0;
        if (!Hcam.empty() && Hcam.rows == 3 && Hcam.cols == 3 &&
            !Hcv.empty() && Hcv.rows == 3 && Hcv.cols == 3) {
            if (Hcam.type() != CV_64FC1) Hcam.convertTo(Hcam, CV_64FC1);
            if (Hcv.type() != CV_64FC1) Hcv.convertTo(Hcv, CV_64FC1);

            auto flag = invert(Hcam, Hcam_inv, DECOMP_LU);
            if (flag) {
                g_logger.log(LogLevel::DEBUG, "Inversion successful.");
                Mat Hobj = Hcam_inv * Hcv;
                //Hobj가 단위 행렬에 가까운지 확인하여 어느 호모그래피가 더 정확한지 판단
                Mat I = Mat::eye(Hobj.rows, Hobj.cols, CV_64FC1);
                Mat Idiff;
                absdiff(Hobj, I, Idiff);
                error = sum(Idiff)[0];
            } else {
                g_logger.log(LogLevel::ERR, "Inversion failed.");
            }
        }
        
        // Hcv 변환 좌표 면적 90% 초과 검사
        auto isAreaOverRatio = [](const vector<vector<CoreTypes::coord>>& boxes, int img_w, int img_h, double ratio_limit) {
            double total_area = 0.0;
            for (const auto& box : boxes) {
                if (box.size() < 4) continue;
                vector<Point2f> pts;
                for (const auto& pt : box) {
                    pts.emplace_back(static_cast<float>(pt.x), static_cast<float>(pt.y));
                }
                total_area += contourArea(pts);
            }
            double img_total_area = static_cast<double>(img_w) * img_h;
            return (total_area >= img_total_area * ratio_limit);
        };

        bool cv_area_exploded = isAreaOverRatio(homographyCordsCV, simgSize.width, simgSize.height, 0.9);

        iou_val = Spec::iou(homographyCords, homographyCordsCV);

        bool select_hcam = !hcv_valid || (iou_val < 0.5) || cv_area_exploded;
        if (!select_hcam) {
            select_hcam = (error < diff_threshold);
        }

        comp_result = select_hcam ? firstWarp_Hcam_resized : firstWarp_Hcv_resized;
        chosen_h_name = select_hcam ? "Hcam" : "Hcv";
        final_smaskedData = select_hcam ? smaskedData : smaskedDataCV;
        nextTrackedCoords = select_hcam ? homographyCords : homographyCordsCV;

        rmse_val = Spec::rmse(comp_result, secondCV);

        g_logger.log(LogLevel::DEBUG, "chosen homography: " + chosen_h_name);
        g_logger.log(LogLevel::DEBUG, "RMSE between images: " + to_string(rmse_val));
        g_logger.log(LogLevel::DEBUG, "IOU between masks: " + to_string(iou_val));

        if (final_smaskedData != nullptr && simgSize.width > 0 && simgSize.height > 0) {
            Mat outResultCam = Mat(simgSize.height, simgSize.width, CV_8UC3, final_smaskedData).clone();
            flip(outResultCam, outResultCam, 0);
            string resultCamPath = "output/result_cam/masked_" + secondFrame.stemName + ".jpg";
            imwrite(resultCamPath, outResultCam);
        }
    }

    // ----------------- Image processing-----------------
    if (!fmaskedData || !final_smaskedData || fimgSize.width <= 0 || simgSize.width <= 0) {
        return nextTrackedCoords;
    }

    Mat fMaskedMat = Mat(fimgSize.height, fimgSize.width, CV_8UC3, fmaskedData).clone();
    Mat sMaskedMat = Mat(simgSize.height, simgSize.width, CV_8UC3, final_smaskedData).clone();
    flip(fMaskedMat, fMaskedMat, 0);
    flip(sMaskedMat, sMaskedMat, 0);

    if (fMaskedMat.rows != sMaskedMat.rows) {
        resize(sMaskedMat, sMaskedMat, Size(sMaskedMat.cols, fMaskedMat.rows));
    }

    Mat combined;
    hconcat(fMaskedMat, sMaskedMat, combined);
    
    // 텍스트 처리
    int top_padding = 80;
    int bottom_padding = 50;
    Mat canvas;
    copyMakeBorder(combined, canvas, top_padding, bottom_padding, 0, 0, BORDER_CONSTANT, Scalar(30, 30, 30));

    char top_metric_text[256];
    snprintf(top_metric_text, sizeof(top_metric_text), 
             "[%s -> %s] RMSE: %.4f | IOU: %.4f | Selected: %s", 
             firstFrame.stemName.c_str(), secondFrame.stemName.c_str(),
             rmse_val, iou_val, chosen_h_name.c_str());
    putText(canvas, top_metric_text, Point(30, 50), 
            FONT_HERSHEY_SIMPLEX, 0.9, Scalar(0, 255, 255), 2, LINE_AA);
    string left_label = firstFrame.stemName + " (Mask)";
    string right_label = secondFrame.stemName + " (" + chosen_h_name + " Mask)";

    int left_x = fimgSize.width / 4;
    int right_x = fimgSize.width + (simgSize.width / 4);
    int bottom_y = canvas.rows - 20;

    putText(canvas, left_label, Point(left_x, bottom_y), 
            FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255, 255, 255), 2, LINE_AA);
    putText(canvas, right_label, Point(right_x, bottom_y), 
            FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255, 255, 255), 2, LINE_AA);

    // 결과 이미지 저장
    fs::create_directories("output/resultimages");
    string outName = "output/resultimages/summary_" + firstFrame.stemName + "_" + secondFrame.stemName + ".jpg";
    imwrite(outName, canvas);

    return nextTrackedCoords;
}

int main() {
    string sceneDir = "input/scene";
    string dataDir = "input/data";
    string boxDir = "input/boxes";
    string resultDir = "output/resultimages";
    string resultCamDir = "output/result_cam";
    std::error_code ec;
    fs::create_directories(resultDir, ec);
    fs::create_directories(resultCamDir, ec);
    vector<fs::path> sceneFiles;

    if (fs::exists(sceneDir)) {
        for (const auto& entry : fs::directory_iterator(sceneDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".bmp") {
                sceneFiles.push_back(entry.path());
            }
        }
    }
    sort(sceneFiles.begin(), sceneFiles.end(), compareNatural);

    vector<FramePair> frames;
    for (const auto& p : sceneFiles) {
        FramePair fp;
        fp.imgPath = p.string();
        fp.stemName = p.stem().string();
        fp.txtPath = dataDir + "/" + fp.stemName + ".txt";
        fp.boxPath = boxDir + "/" + fp.stemName + "_box.txt";
        frames.push_back(fp);
    }

    int total_pairs = static_cast<int>(frames.size()) - 1;

    vector<vector<CoreTypes::coord>> trackedCoords;
    for (int i = 0; i < total_pairs; ++i) {
        g_logger.log(LogLevel::INFO, "processing frame : " + frames[i].stemName + " -> " + frames[i + 1].stemName);        
        trackedCoords = instance(frames[i], frames[i + 1], i + 1, trackedCoords);
    }

    return 0;
}