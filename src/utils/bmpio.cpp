#include "utils/bmpio.hpp"


using namespace std;

namespace BmpIO {
BYTE* loadfile(const std::string& filename, CoreTypes::ImgSize& imgSize, 
               BITMAPFILEHEADER* bmpFHeader, BITMAPINFOHEADER* bmpIHeader)
               {
                    timer t("bmpio: Loading BMP file : " + filename);
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
void savefile(const std::string& filename, const CoreTypes::ImgSize imgSize, 
              const BITMAPFILEHEADER bmpFHeader, const BITMAPINFOHEADER bmpIHeader, 
              const BYTE* imgData)
              {
                timer t("bmpio: Saving BMP file : " + filename);
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
} // namespace BmpIO