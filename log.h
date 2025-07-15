#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <string>
#include "module.h"
enum LogType
{
    info, warning, error
};

extern string log_file;
static map<LogType, string> LogTypeName ={
        {LogType::info, "INFO"},
        {LogType::warning, "WARNING"},
        {LogType::error, "ERROR"}
};

void init_log();
void log(LogType type, string s);
void logFile(LogType type);
string get_log_file();

template<typename... Args>
void log(LogType type, Args... args) {
    ostringstream oss;
    oss << "[" << LogTypeName[type] << "] ";
    ((oss << args), ... );
    fstream fout = fstream(log_file.c_str(), ios::out | ios::app);
    fout << oss.str() << endl;
    fout.close();
    cout << oss.str() << endl;
}

template<typename... Args>
void ERROR(Args... args) {
    ostringstream oss;
    oss << "[ERROR] ";
    ((oss << args), ... );
    fstream fout = fstream(log_file.c_str(), ios::out | ios::app);
    fout << oss.str() << endl;
    fout.close();
    cout << oss.str() << endl;
}

template<typename... Args>
void INFO(Args... args) {
    ostringstream oss;
    oss << "[INFO] ";
    ((oss << args), ... );
    fstream fout = fstream(log_file.c_str(), ios::out | ios::app);
    fout << oss.str() << endl;
    fout.close();
    cout << oss.str() << endl;
}

template<typename... Args>
void WARNING(Args... args) {
    ostringstream oss;
    oss << "[WARNING] ";
    ((oss << args), ... );
    fstream fout = fstream(log_file.c_str(), ios::out | ios::app);
    fout << oss.str() << endl;
    fout.close();
    cout << oss.str() << endl;
}

void PRINT(Ctx &ctx, Sol &sol);

template<typename... Args>
void RESULT(Ctx&ctx, Sol& sol, Args... args) {
    ostringstream oss;
    oss << "[WARNING] ";
    ((oss << args), ... );
    fstream fout = fstream(log_file.c_str(), ios::out | ios::app);
    fout << oss.str() << endl;
    fout.close();
    cout << oss.str() << endl;
    PRINT(ctx, sol);
}