#include "evaluation.h"
#include "log.h"
// utilization + accepatance

int all_n = 0;
int acc_n = 0;
vector<string> A;

string get_file_name(Ctx& ctx) {
    string file;
    if (ctx.param["log"] == "default") {
        file = get_log_file();
    } else if (ctx.param["log"] == "nested") {
        file = ctx.param["optimizer"] + "_" + ctx.param["test_suit"].substr(ctx.param["test_suit"].length() - 2, 2);
    } else if (ctx.param["log"] == "abc") {
        int a = (int)(stod(ctx.param["alpha"]) * 10);
        if (a == 10)
            file += std::to_string(a);
        else
            file += "0" + std::to_string(a);
        file += "-";
        a = (int)(stod(ctx.param["beta"]) * 10);
        if (a == 10)
            file += std::to_string(a);
        else
            file += "0" + std::to_string(a);
        file += "-";
        a = (int)(stod(ctx.param["gamma"]) * 10);
        if (a == 10)
            file += std::to_string(a);
        else
            file += "0" + std::to_string(a);
    } else if(ctx.param["log"] == "test") {
        file = ctx.param["test_suit"];
    }
    return file;
}

namespace Evaluation{
    void init() {
        all_n = 0;
        acc_n = 0;
        A.clear();
    }

    void result(Ctx& ctx) {
        string file = get_file_name(ctx);
        fstream fout = fstream("./result/" + file + ".txt", ios::out | ios::trunc);
        if (!fout) {
            ERROR("open file wrong :./result/" + file + ".txt");
        }
        fout << "Result file:" << file << endl;
        cout << "Result file:" << file << endl;
        for (auto &t: ctx.task_time) {
            fout << t.first << " consumes " << t.second << endl;
            cout << t.first << " consumes " << t.second << endl;
        }
        cout << "FPGA:" << ctx.param["fpga"] << endl;
        fout << "Optimizer:" << ctx.param["optimizer"] << endl << "Task suit:" << ctx.param["test_suit"] << endl;
        fout << "Relaxation:" << ctx.param["relaxation"] << endl << "select:" << ctx.param["select"] << endl << "Mode:" << ctx.param["mode"] << endl;
        fout << "Metric:" << ctx.param["alpha"] << "," << ctx.param["beta"] << "," << ctx.param["gamma"] << endl;
        fout << "Last task ends at:" << ctx.diagrams.rbegin()->first << endl;
        fout << "Processing Time consuming:" << std::to_string((double)ctx.time/1e6) << endl;
        fout << "Utilization:" << Evaluation::utilization(ctx) << "        Acceptance Rate (" << Evaluation::acc() << "/" << Evaluation::all() << "):" << Evaluation::acceptance()
             <<"        Waste Rate:" << Evaluation::wastes(ctx) << endl;

        cout << "Optimizer:" << ctx.param["optimizer"] << endl << "Task suit:" << ctx.param["test_suit"] << endl;
        cout << "Relaxation:" << ctx.param["relaxation"] << endl << "select:" << ctx.param["select"] << endl << "Mode:" << ctx.param["mode"] << endl;
        cout << "Metric:" << ctx.param["alpha"] << "," << ctx.param["beta"] << "," << ctx.param["gamma"] << endl;
        cout << "Last task ends at:" << ctx.diagrams.rbegin()->first << endl;
        cout << "Processing Time consuming:" << std::to_string((double)ctx.time/1e6) << endl;
        cout << "Utilization:" << Evaluation::utilization(ctx) << "        Acceptance Rate (" << Evaluation::acc() << "/" << Evaluation::all() << "):" << Evaluation::acceptance()
             <<"        Waste Rate:" << Evaluation::wastes(ctx) << endl;
    }


    void ACC(long tin, long e) {
        A.push_back("(" + to_string(tin) + "," + to_string(e)  + ")");
    }
    void AC() {
        fstream fout("acc.txt", ios::out | ios::trunc);
        for (auto& a: A)
            fout << a << endl;
    }

    void time_trend(Ctx& ctx) {
        for (auto &t: ctx.task_time)
            INFO(t.first, " consumes ", t.second);
    }

    void accept() {
        all_n++;
        acc_n++;
    }

    void reject() {
        all_n++;
    }

    double acceptance() {
        return (double)acc_n / all_n;
    }

    double utilization(Ctx& ctx) {
        auto next = ctx.diagrams.begin();
        auto curr = next;
        next++;
        double re = 0.0;
        long period = 0;
//        fstream fout = fstream("evaluation.txt", ios::out | ios::trunc);
        for (;next != ctx.diagrams.end(); curr++, next++) {
            period += next->first - curr->first;
            re += curr->second->get_utilization() * (next->first - curr->first);
//            fout << std::to_string(curr->first) << " time " << std::to_string(period) << " adding " << std::to_string(re) << endl;
        }
//        fout << "last: " << std::to_string(re/period) << endl;
//        fout.close();
        return re / period;
    }

    double wastes(Ctx& ctx) {
        auto next = ctx.diagrams.begin();
        auto curr = next;
        next++;
        double re = 0.0;
        long period = 0;
        for (;next != ctx.diagrams.end(); curr++, next++) {
            period += next->first - curr->first;
            re += curr->second->get_wastes() * (next->first - curr->first);
        }
        return re / period;
    }

    int acc() {
        return acc_n;
    }

    int all() {
        return all_n;
    }
}