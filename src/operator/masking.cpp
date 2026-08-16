#include "operator/masking.hpp"
#include <algorithm>
using namespace std;

namespace Operator
{
    vector<vector<CoreTypes::coord>> loadcord()
    {
        timer t("masking: loadcord method");
        ifstream file("input/detected_boxes.txt");
        if (!file.is_open()) {
            cerr << "Error opening cord file: cord.txt" << endl;
            return {};
        }
        int n;
        file >> n;
        vector<vector<CoreTypes::coord>> ret(n);
        for (int i = 0; i < n; i++) {
            ret[i].resize(4);
            //ul, ur, dl, dr
            file >> ret[i][0].x >> ret[i][0].y; // ul
            file >> ret[i][1].x >> ret[i][1].y; // ur
            file >> ret[i][2].x >> ret[i][2].y; // dl
            file >> ret[i][3].x >> ret[i][3].y; // dr
        }
        file.close();
        return ret;
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
    BYTE * makeQuadMask(struct CoreTypes::ImgSize imgSize, const vector<vector<CoreTypes::coord>>& maskingCords)
    {
        timer t("masking: makeQuadMask method");
        int totalBytes = imgSize.width * imgSize.height * 3;
        BYTE * maskdata = new BYTE[totalBytes];
        fill(maskdata, maskdata + totalBytes, 255);

        for (const auto& cords : maskingCords)
        {
            if (cords.size() < 4) continue;
            int min_x = min({cords[0].x, cords[1].x, cords[2].x, cords[3].x});
            int max_x = max({cords[0].x, cords[1].x, cords[2].x, cords[3].x});
            int min_y = min({cords[0].y, cords[1].y, cords[2].y, cords[3].y});
            int max_y = max({cords[0].y, cords[1].y, cords[2].y, cords[3].y});

            int start_x = max(0, min(min_x, imgSize.width));
            int end_x   = max(0, min(max_x, imgSize.width));
            int start_y = max(0, min(min_y, imgSize.height));
            int end_y   = max(0, min(max_y, imgSize.height));

            for (int i = start_y; i < end_y; i++)
            {
                for (int j = start_x; j < end_x; j++)
                {
                    int idx = BmpIO::cordtoidx(imgSize, {j, i});
                    if (idx >= 0 && idx + 2 < totalBytes) {
                        maskdata[idx] = 0;
                        maskdata[idx + 1] = 0;
                        maskdata[idx + 2] = 0;
                    }
                }
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
    BYTE * masking(const BYTE * source, const BYTE * mask, struct CoreTypes::ImgSize imgSize)
    {
        timer t("masking: masking method");
        BYTE * ret = new BYTE[imgSize.width * imgSize.height * 3];
        for(int i = 0; i < imgSize.width * imgSize.height * 3; i++)
        {
            ret[i] = source[i] & mask[i];
        }
        return ret;
    }
}