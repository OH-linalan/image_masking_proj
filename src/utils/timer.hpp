#pragma once

#include <chrono>
#include <string>
#include <iostream>

#include "logger.hpp"

using namespace std;

class timer
{
private:
    chrono::high_resolution_clock::time_point start_time;
    string name;

public:
    timer(const string& name)
        : start_time(chrono::high_resolution_clock::now()),
          name(name)
    {
    }
    ~timer()
    {
        auto end_time = chrono::high_resolution_clock::now();
        auto duration_us = chrono::duration_cast<chrono::microseconds>(end_time - start_time).count();
        g_logger.log(LogLevel::DEBUG, "[" + name + "] Execution time: " + to_string(duration_us) + " us (" + to_string(duration_us / 1000.0) + " ms)");
    }
};