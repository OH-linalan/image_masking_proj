#include <iostream>
#include <string>
#include <fstream>
#include <Windows.h>
#include <cstdio>
#include <cstdint>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include <vector>
#include <tuple>
#include <chrono>
using namespace std;
using namespace cv;
struct size
{
    int width;
    int height;
};
struct coord
{
    int x;
    int y;
};
class matrix
{
public:
    int row;
    int colm;
    double ** data;
    matrix(int r, int c) : row(r), colm(c) {
        data = new double*[row];
        for (int i = 0; i < row; i++) {
            data[i] = new double[colm];
        }
    }
    matrix(int r, int c, double ** d) : row(r), colm(c), data(d) {}
    matrix innerP(const matrix& other)
    {
        if (colm != other.row) {
            throw invalid_argument("ERROR: invalid product!");
        }
        matrix result(row, other.colm);
        for (int i = 0; i < row; i++) {
            for (int j = 0; j < other.colm; j++) {
                result.data[i][j] = 0;
                for (int k = 0; k < colm; k++) {
                    result.data[i][j] += data[i][k] * other.data[k][j];
                }
            }
        }
        return result;
    }
    matrix transpose()
    {
        matrix result(colm, row);
        for (int i = 0; i < row; i++) {
            for (int j = 0; j < colm; j++) {
                result.data[j][i] = data[i][j];
            }
        }
        return result;
    }
    //K의 역행렬, 특수한 경우로 일반적 역행렬에서 사용 금지 3x3
    matrix Kinv()
    {
        double ** invdata = new double*[3];
        for (int i = 0; i < 3; i++) {
            invdata[i] = new double[3];
        }
        invdata[0][0] = 1/data[0][0];
        invdata[0][1] = 0;
        invdata[0][2] = -data[0][2]/data[0][0];
        invdata[1][0] = 0;
        invdata[1][1] = 1/data[1][1];
        invdata[1][2] = -data[1][2]/data[1][1];
        invdata[2][0] = 0;
        invdata[2][1] = 0;
        invdata[2][2] = 1;
        return matrix(3, 3, invdata);
    }
    void print()
    {
        for (int i = 0; i < row; i++) {
            for (int j = 0; j < colm; j++) {
                cout << data[i][j] << " ";
            }
            cout << endl;
        }
    }
    ~matrix() {
        for (int i = 0; i < row; i++) {
            delete[] data[i];
        }
        delete[] data;
    }
};
struct calib
{
    matrix * K;
    matrix * R;
    matrix * T;
};
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
//좌표를 이미지 데이터 배열의 인덱스로 변환하는 함수
int cordtoidx(const struct size& imgSize, const struct coord& c){
        int temp = imgSize.height - 1 - c.y;
        return (temp * imgSize.width + c.x) * 3;
    }
/**
 * @brief BMP 파일을 로드하여 픽셀 데이터를 메모리에 할당합니다.
 * * @param filename 로드할 BMP 파일의 경로
 * @param[out] imgSize 로드된 이미지의 가로, 세로 크기가 저장될 구조체
 * @param[out] bmpFHeader 파일 헤더 정보를 담을 구조체 포인터
 * @param[out] bmpIHeader 정보 헤더 정보를 담을 구조체 포인터
 * @return 성공 시 할당된 픽셀 데이터의 포인터(BYTE*), 실패 시 nullptr 반환
 * @note 반환된 포인터는 사용 후 반드시 delete[]를 통해 메모리를 해제해야 합니다.
 */
BYTE * loadfile(const string filename, struct size& imgSize,BITMAPFILEHEADER* bmpFHeader, BITMAPINFOHEADER * bmpIHeader )
{
    printf("Loading file: %s\n", filename.c_str());
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return nullptr;
    }
    file.read((char*)bmpFHeader, sizeof(BITMAPFILEHEADER));
    file.read((char*)bmpIHeader, sizeof(BITMAPINFOHEADER));
    if(bmpFHeader->bfType != 0x4D42) {
        cerr << "Not a valid BMP file: " << filename << endl;
        return nullptr;
    }
    imgSize.width = bmpIHeader->biWidth;
    imgSize.height = bmpIHeader->biHeight;
    printf("Width: %d, Height: %d\n", imgSize.width, imgSize.height);
    uint32_t dataSize = bmpIHeader->biSizeImage;
    if (dataSize == 0) {
        dataSize = imgSize.width * imgSize.height * (bmpIHeader->biBitCount / 8);
    }
    file.seekg(bmpFHeader->bfOffBits, std::ios::beg);
    BYTE* imgData = new BYTE[dataSize];
    file.read((char*)imgData, dataSize);
    file.close();
    return imgData;
}
/**
 * @brief 메모리에 있는 이미지 데이터를 BMP 파일로 저장합니다.
 * * @param filename 저장할 파일의 이름 (경로 포함)
 * @param imgSize 이미지의 가로, 세로 크기 정보
 * @param bmpFHeader 파일에 기록할 BITMAPFILEHEADER 구조체
 * @param bmpIHeader 파일에 기록할 BITMAPINFOHEADER 구조체
 * @param imgData 파일에 쓸 픽셀 데이터 배열의 포인터
 * @return void
 * * @details 
 * 제공된 헤더 정보를 바탕으로 BMP 파일을 생성합니다. 
 */
void savefile(const string filename, const struct size imgSize, const BITMAPFILEHEADER bmpFHeader, const BITMAPINFOHEADER bmpIHeader, const BYTE* imgData)
{
    ofstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Error opening file for writing: " << filename << endl;
        return;
    }
    file.write((char*)&bmpFHeader, sizeof(BITMAPFILEHEADER));
    file.write((char*)&bmpIHeader, sizeof(BITMAPINFOHEADER));
    file.seekp(bmpFHeader.bfOffBits, std::ios::beg);
    uint32_t dataSize = bmpIHeader.biSizeImage;
    if (dataSize == 0) {
        dataSize = imgSize.width * imgSize.height * (bmpIHeader.biBitCount / 8);
    }
    file.write((char*)imgData, dataSize);
    file.close();
}
/**
 * @brief 사각형 영역을 지정하여 마스킹을 위한 이미지 마스크 데이터를 생성합니다.
 * 
 * @param imgSize       대상 이미지의 해상도 정보 (가로, 세로 크기)
 * @param maskingCords  사각형의 꼭짓점 좌표 배열 (순서: [0]=ul, [1]=ur, [2]=dl, [3]=dr)
 * 
 * @return BYTE*        동적 할당된 마스크 데이터 바이트 배열 포인터 
 *                      (사용 후 반드시 delete[]로 메모리를 해제해야 합니다.)
 * @see masking
 */
BYTE * makeQuadMask(struct size imgSize, const coord * maskingCords)
{
    BYTE * maskdata = new BYTE[imgSize.width * imgSize.height * 3];
    fill(maskdata, maskdata + imgSize.width * imgSize.height * 3, 255);
    //ul, ur, dl, dr
    for (int i = maskingCords[0].y; i < maskingCords[3].y; i++)
    {
        for (int j = maskingCords[0].x; j < maskingCords[3].x; j++)
        {
            int idx = cordtoidx(imgSize, {j, i});
            maskdata[idx] = 0;
            maskdata[idx + 1] = 0;
            maskdata[idx + 2] = 0;
        }
    }
    return maskdata;
}
/**
 * @brief 원본 이미지 데이터와 마스크 데이터를 비트 연산으로 결합하여 마스킹을 수행합니다.
 * 
 * @param source    원본 이미지의 픽셀 바이트 배열 포인터
 * @param mask      makeQuadMask 등을 통해 생성된 마스크 데이터 바이트 배열 포인터
 * @param imgSize   이미지의 해상도 정보 (가로, 세로 크기)
 * 
 * @return BYTE*    마스킹 처리가 완료된 새로운 이미지의 동적 할당된 바이트 배열 포인터
 *                  (사용 후 반드시 delete[]로 메모리를 해제해야 합니다.)
 * 
 * @note mask 배열의 배경은 255, 가릴 영역은 0으로 채워져 있어야 원본이 왜곡되지 않습니다.
 * @see makeQuadMask
 */
BYTE * masking(const BYTE * source, const BYTE * mask, struct size imgSize)
{
    BYTE * ret = new BYTE[imgSize.width * imgSize.height * 3];
    for(int i = 0; i < imgSize.width * imgSize.height * 3; i++)
    {
        ret[i] = source[i] & mask[i];
    }
    return ret;
}
struct calib Loadcalib(const string filename)
{
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening matrix file: " << filename << endl;
        return calib{nullptr, nullptr, nullptr};
    }
    double ** Kdata = new double*[3];
    for (int i = 0; i < 3; i++) {
        Kdata[i] = new double[3];
    }
    double ** Rdata = new double*[3];
    for (int i = 0; i < 3; i++) {
        Rdata[i] = new double[3];
    }
    double ** Tdata = new double*[3];
    for (int i = 0; i < 3; i++) {   
        Tdata[i] = new double[1];
    }
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            file >> Kdata[i][j];
        }
    }
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            file >> Rdata[i][j];
        }
    }
    for (int i = 0; i < 3; i++) {
        file >> Tdata[i][0];
    }
    file.close();
    matrix * K = new matrix(3, 3, Kdata);
    matrix * R = new matrix(3, 3, Rdata);
    matrix * T = new matrix(3, 1, Tdata);
    return calib{K, R, T};
}
matrix findH(calib c, calib c2)
{
    matrix Rdiff = c2.R->innerP(c.R->transpose());
    matrix H = c.K->innerP(Rdiff).innerP(c.K->Kinv());
    return H;
}
coord * homography(matrix H, const coord * c){
    coord * ret = new coord[4];
    for (int i = 0; i < 4; i++) {
        matrix point(3, 1);
        point.data[0][0] = c[i].x;
        point.data[1][0] = c[i].y;
        point.data[2][0] = 1;
        matrix project = H.innerP(point);
        double temp = project.data[2][0];
        if(temp != 0) {
            ret[i].x = project.data[0][0] / temp;
            ret[i].y = project.data[1][0] / temp;
        } else {
            ret[i].x = c[i].x;
            ret[i].y = c[i].y;
        }
    }
    return ret;
}
Mat LoadCV(const string filename)
{
    Mat img = imread(filename);
    if(img.empty())
    {
        std::cout<<"Can't find image" << std::endl;
        return Mat();
    }
    return img;
}
struct orbData cvORB(const Mat& prev, const Mat& next)
{
    Ptr<FeatureDetector> detector = ORB::create();
    vector<KeyPoint> prevKeypoints, nextKeypoints;
    Mat prevDesc, nextDesc;
    detector->detectAndCompute(prev, noArray(), prevKeypoints, prevDesc);
    detector->detectAndCompute(next, noArray(), nextKeypoints, nextDesc);
    Ptr<DescriptorMatcher> matcher = BFMatcher::create(NORM_HAMMING);
    vector<vector<DMatch>> matches;
    matcher->knnMatch(prevDesc, nextDesc, matches, 2);
    return orbData{prevKeypoints, nextKeypoints, matches};
}
struct cvHomographyResult cvHomography(const orbData& data, double ratio)
{
    //Lowe's ratio test
    vector<DMatch> acceptMatch;
    for(const auto& m : data.matches)
    {
        if(m[0].distance< ratio * m[1].distance)
        {
            acceptMatch.push_back(m[0]);
        }
    }
    vector<Point2f> prevKeypoints, nextKeypoints;
    for(const auto& m : acceptMatch)
    {
        prevKeypoints.push_back(data.prevKeypoint[m.queryIdx].pt);
        nextKeypoints.push_back(data.nextKeypoint[m.trainIdx].pt);
    }
    Mat H = findHomography(prevKeypoints, nextKeypoints, RANSAC);
    cout << "Number of accepted matches: " << acceptMatch.size() << endl;
    cout << "Homography matrix:" << endl;
    cout << H << endl;
    return cvHomographyResult{H, prevKeypoints, nextKeypoints, acceptMatch};
}
double rmse(Mat img1, Mat img2)
{
    Mat diff;
    absdiff(img1, img2, diff);
    diff.convertTo(diff, CV_32F);
    Mat sq = diff.mul(diff);
    Scalar sum = cv::sum(sq);
    double total = sum[0] + sum[1] + sum[2];
    double elem = img1.rows * img1.cols * img1.channels();
    return sqrt(total / elem);
}
double iou(coord * c1, coord * c2)
{
    //마스크의 좌표를 이용하여 사각형 영역 계산
    int c1_xmin = min({c1[0].x, c1[1].x, c1[2].x, c1[3].x});
    int c1_xmax = max({c1[0].x, c1[1].x, c1[2].x, c1[3].x});
    int c1_ymin = min({c1[0].y, c1[1].y, c1[2].y, c1[3].y});
    int c1_ymax = max({c1[0].y, c1[1].y, c1[2].y, c1[3].y});

    int c2_xmin = min({c2[0].x, c2[1].x, c2[2].x, c2[3].x});
    int c2_xmax = max({c2[0].x, c2[1].x, c2[2].x, c2[3].x});
    int c2_ymin = min({c2[0].y, c2[1].y, c2[2].y, c2[3].y});
    int c2_ymax = max({c2[0].y, c2[1].y, c2[2].y, c2[3].y});
    //사각형 영역을 계산
    Rect c1_r(c1_xmin, c1_ymin, c1_xmax - c1_xmin, c1_ymax - c1_ymin);
    Rect c2_r(c2_xmin, c2_ymin, c2_xmax - c2_xmin, c2_ymax - c2_ymin);
    //교집합 영역 계산
    Rect inter = c1_r & c2_r;
    double interArea = inter.area();
    //합집합 영역 계산
    double unionArea = c1_r.area() + c2_r.area() - interArea;
    //에러처리
    if (unionArea == 0) {
        return 0.0;
    }
    return interArea / unionArea;
}
int main() {
    //시작 시간
    auto start = chrono::high_resolution_clock::now();

    //first.bmp 로드
    struct size fimgSize;
    BITMAPFILEHEADER fbmpFHeader;
    BITMAPINFOHEADER fbmpIHeader;
    auto fimgData = loadfile("input/first.bmp", fimgSize, &fbmpFHeader, &fbmpIHeader);

    struct size simgSize;
    BITMAPFILEHEADER sbmpFHeader;
    BITMAPINFOHEADER sbmpIHeader;
    auto simgData = loadfile("input/second.bmp", simgSize, &sbmpFHeader, &sbmpIHeader);
    
    //마스킹 좌표 설정, 마스크 데이터 생성
    coord maskingCoord[4] = {{514, 163}, {733, 163}, {514, 421}, {733, 421}};
    auto maskData = makeQuadMask(fimgSize, maskingCoord);

    //마스킹 수행
    auto fmaskedData = masking(fimgData, maskData, fimgSize);

    //output 폴더에 foutput.bmp로 저장
    savefile("output/first_masked_output.bmp", fimgSize, fbmpFHeader, fbmpIHeader, fmaskedData);

    auto sfmaskedData = masking(simgData, maskData, simgSize);
    savefile("output/second_masked_output_firstdata.bmp", simgSize, sbmpFHeader, sbmpIHeader, sfmaskedData);

    //호모그래피 계산
    auto fdata = Loadcalib("input/firstdata.txt");
    auto sdata = Loadcalib("input/seconddata.txt");
    auto H = findH(fdata, sdata);
    cout << "homography matrix:" << endl;
    H.print();

    //호모그래피 행렬을 마스킹 좌표에 적용
    coord * homographyCords = homography(H, maskingCoord);
    cout << "original coordinates:" << endl;
    for (int i = 0; i < 4; i++) {
        cout << "x: " << maskingCoord[i].x << ", y: " << maskingCoord[i].y << endl;
    }
    cout << "homography coordinates:" << endl;
    for (int i = 0; i < 4; i++) {
        cout << "x: " << homographyCords[i].x << ", y: " << homographyCords[i].y << endl;
    }

    //변환된 좌표를 이용하여 두 번째 이미지에 마스킹 적용    
    auto smaskData = makeQuadMask(simgSize, homographyCords);
    auto smaskedData = masking(simgData, smaskData, simgSize);

    //output 폴더에 soutput.bmp로 저장
    savefile("output/second_masked_output_homography.bmp", simgSize, sbmpFHeader, sbmpIHeader, smaskedData);

    //------------------------OPENCV------------------------
    //OpenCV로 이미지 로드
    auto firstCV = LoadCV("input/first.bmp");
    auto secondCV = LoadCV("input/second.bmp");
    //ORB 특징점 검출 및 매칭
    auto orbResult = cvORB(firstCV, secondCV);
    cout << "Number of keypoints in first image: " << orbResult.prevKeypoint.size() << endl;
    cout << "Number of keypoints in second image: " << orbResult.nextKeypoint.size() << endl;
    cout << "Number of matches: " << orbResult.matches.size() << endl;
    //매칭 결과 시각화
    Mat matchImg;
    drawMatches(firstCV, orbResult.prevKeypoint, secondCV, orbResult.nextKeypoint, orbResult.matches, matchImg);
    imwrite("output/ORB_matches.png", matchImg);

    auto HcvResult = cvHomography(orbResult, 0.75);
    //ratio test를 적용하여 accept된 매칭을 시각화
    Mat acceptMatchImg;
    drawMatches(firstCV, orbResult.prevKeypoint, secondCV, orbResult.nextKeypoint, HcvResult.acceptMatch, acceptMatchImg);
    imwrite("output/ORB_accepted_matches.png", acceptMatchImg);

    auto Hcv = HcvResult.H;
    double ** HcvData;
    HcvData = new double*[Hcv.rows];
        for (int i = 0; i < Hcv.rows; i++) {
            HcvData[i] = new double[Hcv.cols];
            for (int j = 0; j < Hcv.cols; j++) {
                HcvData[i][j] = Hcv.at<double>(i, j);
            }
        }
    matrix HcvM(Hcv.rows, Hcv.cols, HcvData);

    //호모그래피 행렬을 마스킹 좌표에 적용
    coord * homographyCordsCV = homography(HcvM, maskingCoord);
    cout << "original coordinates:" << endl;
    for (int i = 0; i < 4; i++) {
        cout << "x: " << maskingCoord[i].x << ", y: " << maskingCoord[i].y << endl;
    }
    cout << "homography coordinates:" << endl;
    for (int i = 0; i < 4; i++) {
        cout << "x: " << homographyCordsCV[i].x << ", y: " << homographyCordsCV[i].y << endl;
    }

    //변환된 좌표를 이용하여 두 번째 이미지에 마스킹 적용    
    auto smaskDataCV = makeQuadMask(simgSize, homographyCordsCV);
    auto smaskedDataCV = masking(simgData, smaskDataCV, simgSize);

    //output 폴더에 soutput.bmp로 저장
    savefile("output/second_masked_output_homography_opencv.bmp", simgSize, sbmpFHeader, sbmpIHeader, smaskedDataCV);

    //output/second_masked_output_homography.bmp와 output/second_masked_output_homography_opencv.bmp의 차이점 분석
    Mat homographyOutput = imread("output/second_masked_output_homography.bmp");
    Mat homographyOutputCV = imread("output/second_masked_output_homography_opencv.bmp");
    Mat diff;
    absdiff(homographyOutput, homographyOutputCV, diff);
    imwrite("output/homography_difference.png", diff);
    cout << "Diff : " << sum(diff) << endl;

    //-----------------specification-------------------
    //matrix class H를 OpenCV의 Mat으로 변환
    Mat Hcam = Mat::zeros(fdata.K->row, fdata.K->colm, CV_64FC1);
    {
        matrix H_temp = findH(fdata, sdata);
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
    threshold(gray, thresh, 1, 255, THRESH_BINARY);
    //보정할 영역을 찾아서 crop & resize
    Rect roi = boundingRect(thresh);
    Mat cropped = firstWarp_Hcam(roi);
    Mat firstWarp_Hcam_resized;
    resize(cropped, firstWarp_Hcam_resized, firstCV.size(),0,0,INTER_LINEAR);
    imwrite("output/first_warped_Hcam.bmp", firstWarp_Hcam_resized);
    //first.bmp를 Hcv로 워핑
    Mat firstWarp_Hcv;
    warpPerspective(firstCV, firstWarp_Hcv, Hcv, firstCV.size());
    //워핑된 이미지의 그레이스케일을 구함
    cvtColor(firstWarp_Hcv, gray, COLOR_BGR2GRAY);
    threshold(gray, thresh, 1, 255, THRESH_BINARY);
    //보정할 영역을 찾아서 crop & resize
    roi = boundingRect(thresh);
    cropped = firstWarp_Hcv(roi);
    Mat firstWarp_Hcv_resized;
    resize(cropped, firstWarp_Hcv_resized, firstCV.size(),0,0,INTER_LINEAR);
    imwrite("output/first_warped_Hcv.bmp", firstWarp_Hcv_resized);
    //카메라 파라미터 호모그래피의 역행렬을 구하고 전체 호모그래피와 곱하여 객체의 움직임 호모그래피를 분리함
    Mat Hcam_inv;
    auto flag = invert(Hcam, Hcam_inv, DECOMP_LU);
    if (flag) {
        cout << "Inversion successful." << endl;
    } 
    else {
        cout << "Inversion failed." << endl;
    }
    Mat Hobj = Hcam_inv * Hcv;
    //Hobj가 단위 행렬에 가까운지 확인하여 어느 호모그래피가 더 정확한지 판단
    Mat I = Mat::eye(Hobj.rows, Hobj.cols, CV_64FC1);
    Mat Idiff;
    absdiff(Hobj, I, Idiff);
    double error = sum(Idiff)[0];
    double threshold = 1e-3;
    //error가 threshold보다 작으면 Hcam이 더 정확하다고 판단하여 Hcam으로 워핑한 이미지를 결과로 사용, 
    //그렇지 않으면 Hcv로 워핑한 이미지를 결과로 사용
    Mat comp_result = (error < threshold) ? firstWarp_Hcam_resized : firstWarp_Hcv_resized;

    cout << "RMSE between images: "<< rmse(comp_result, secondCV) << endl;
    cout << "IOU between masks: " << iou(homographyCords, homographyCordsCV) << endl;

    //종료 시간
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Execution time: " << duration.count() << " ms" <<" = "<< duration.count()/1000.0 << " s" << endl;

    delete[] fimgData;
    delete[] simgData;
    delete[] maskData;
    delete[] fmaskedData;
    delete[] homographyCords;
    delete[] sfmaskedData;
    delete[] smaskData;
    delete[] smaskedData;
    delete[] homographyCordsCV;
    return 0;
}