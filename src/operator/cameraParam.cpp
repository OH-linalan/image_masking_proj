#include "operator/cameraParam.hpp"

namespace Operator
{
struct calib Loadcalib(const string filename)
{
    timer t("CameraParam:Loadcalib method : "+ filename);
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
    Math::matrix * K = new Math::matrix(3, 3, Kdata);
    Math::matrix * R = new Math::matrix(3, 3, Rdata);
    Math::matrix * T = new Math::matrix(3, 1, Tdata);
    return calib{K, R, T};
}
Math::matrix findH(calib c, calib c2)
{
    timer t("CameraParam:findH method (Rdiff and Kinv)");
    Math::matrix Rdiff = c2.R->innerP(c.R->transpose());
    Math::matrix H = c.K->innerP(Rdiff).innerP(c.K->Kinv());
    return H;
}
vector<vector<CoreTypes::coord>> homography(Math::matrix H, const vector<vector<CoreTypes::coord>>& c)
{
    timer t("CameraParam:keypoint homography transformation(homography method)");
    vector<vector<CoreTypes::coord>> ret;

    for (size_t obj = 0; obj < c.size(); obj++) {
        if (c[obj].size() < 4) continue;

        vector<CoreTypes::coord> transformed_obj(4);

        for (int i = 0; i < 4; i++) {
            Math::matrix point(3, 1);
            point.data[0][0] = c[obj][i].x;
            point.data[1][0] = c[obj][i].y;
            point.data[2][0] = 1;

            Math::matrix project = H.innerP(point);
            double temp = project.data[2][0];

            if (temp != 0) {
                transformed_obj[i].x = project.data[0][0] / temp;
                transformed_obj[i].y = project.data[1][0] / temp;
            } else {
                transformed_obj[i].x = c[obj][i].x;
                transformed_obj[i].y = c[obj][i].y;
            }
        }
        ret.push_back(transformed_obj);
    }
    return ret;
}
}