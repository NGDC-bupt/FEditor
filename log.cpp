#include "log.h"

using namespace std;

string log_file = "log.txt";
string main_body = "";

string get_log_file () {
    return main_body;
};

void init_log(){
    auto now = chrono::system_clock::now();
    time_t now_time_t = chrono::system_clock::to_time_t(now);
    tm local_tm = *localtime(&now_time_t);
    main_body = to_string(local_tm.tm_year + 1900) + "-" + to_string(local_tm.tm_mon + 1) + "-" + to_string(local_tm.tm_mday)
                + "-" + to_string(local_tm.tm_hour) + "-" + to_string(local_tm.tm_min) + "-" + to_string(local_tm.tm_sec);
    log_file= "./log/" + main_body + ".log";
}

void log(LogType type, string s) {
    fstream fout = fstream(log_file.c_str(), ios::out | ios::app);
    fout << s << endl;
    fout.close();
}

void logFile(LogType type) {
    fstream fout = fstream(log_file.c_str(), ios::out | ios::app);
    fout << "[INFO] " << "log_file:" << log_file << endl;
    fout.close();
    cout << "[INFO] " << "log_file:" << log_file << endl;
}

void PRINT(Ctx &ctx, Sol &sol) {
    fstream fout = fstream(log_file.c_str(), ios::out | ios::app);
    fout << endl << "Diagrams show:" << endl;
    for (auto& ele: ctx.diagrams) {
        fout << "(" << ele.first << ", " << ele.second->utility << ", " << ele.second->players << ")" << endl;
        fout << ele.second->to_string() << endl;
    }
    fout << endl;
    fout.close();
}
