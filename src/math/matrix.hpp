#pragma once
#include <stdexcept>
#include <iostream>

#include "utils/timer.hpp"

namespace Math {
    class matrix {
    public:
        int row;
        int colm;
        double** data;

        matrix(int r, int c);
        matrix(int r, int c, double** d);
        
        matrix(const matrix& other);
        matrix& operator=(const matrix& other);
        matrix(matrix&& other) noexcept;
        matrix& operator=(matrix&& other) noexcept;

        ~matrix();

        matrix innerP(const matrix& other);
        matrix transpose();
        matrix Kinv();
        void print();
    };
}//namespace Math