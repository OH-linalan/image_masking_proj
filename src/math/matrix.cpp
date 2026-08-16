#include "math/matrix.hpp"
#include <iostream>
#include <stdexcept>
#include <utility>

using namespace std;

namespace Math
{
    matrix::matrix(int r, int c) : row(r), colm(c) {
        data = new double*[row];
        for (int i = 0; i < row; i++) {
            data[i] = new double[colm]();
        }
    }

    matrix::matrix(int r, int c, double ** d) : row(r), colm(c), data(d) {}

    matrix::matrix(const matrix& other) : row(other.row), colm(other.colm) {
        data = new double*[row];
        for (int i = 0; i < row; i++) {
            data[i] = new double[colm];
            for (int j = 0; j < colm; j++) {
                data[i][j] = other.data[i][j];
            }
        }
    }

    matrix& matrix::operator=(const matrix& other) {
        if (this == &other) return *this;

        if (data) {
            for (int i = 0; i < row; i++) {
                delete[] data[i];
            }
            delete[] data;
        }

        row = other.row;
        colm = other.colm;
        data = new double*[row];
        for (int i = 0; i < row; i++) {
            data[i] = new double[colm];
            for (int j = 0; j < colm; j++) {
                data[i][j] = other.data[i][j];
            }
        }
        return *this;
    }

    matrix::matrix(matrix&& other) noexcept : row(other.row), colm(other.colm), data(other.data) {
        other.row = 0;
        other.colm = 0;
        other.data = nullptr;
    }

    matrix& matrix::operator=(matrix&& other) noexcept {
        if (this == &other) return *this;

        if (data) {
            for (int i = 0; i < row; i++) {
                delete[] data[i];
            }
            delete[] data;
        }

        row = other.row;
        colm = other.colm;
        data = other.data;

        other.row = 0;
        other.colm = 0;
        other.data = nullptr;

        return *this;
    }

    matrix matrix::innerP(const matrix& other) {
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

    matrix matrix::transpose() {
        matrix result(colm, row);
        for (int i = 0; i < row; i++) {
            for (int j = 0; j < colm; j++) {
                result.data[j][i] = data[i][j];
            }
        }
        return result;
    }

    matrix matrix::Kinv() {
        double ** invdata = new double*[3];
        for (int i = 0; i < 3; i++) {
            invdata[i] = new double[3];
        }
        invdata[0][0] = 1.0 / data[0][0];
        invdata[0][1] = 0;
        invdata[0][2] = -data[0][2] / data[0][0];
        invdata[1][0] = 0;
        invdata[1][1] = 1.0 / data[1][1];
        invdata[1][2] = -data[1][2] / data[1][1];
        invdata[2][0] = 0;
        invdata[2][1] = 0;
        invdata[2][2] = 1;
        return matrix(3, 3, invdata);
    }

    void matrix::print() {
        for (int i = 0; i < row; i++) {
            for (int j = 0; j < colm; j++) {
                cout << data[i][j] << " ";
            }
            cout << endl;
        }
    }

    matrix::~matrix() {
        if (data != nullptr) {
            for (int i = 0; i < row; i++) {
                if (data[i] != nullptr) {
                    delete[] data[i];
                }
            }
            delete[] data;
            data = nullptr;
        }
    }
}