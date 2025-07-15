#pragma once

#include "module.h"
#include <fstream>
#include <memory>
#include <string>
#include <queue>
#include <cmath>
using namespace std;

// tasks (t_k, e_k, r_k)
class Task{
public:
    long label; // task_seq
    long t; // try relaxation and delay
    long ot; // original t to print and reset
    long best_t; // final solution
    long e; // execution time
    long r[TILE_TYPE]; // resource requirement
    long ddl; // latest timestamp to start
    long area; // sum \vec{r}
    double credit; // only when return
    Task(long li, long ti, long ei, long ddli, long clb, long ram, long dsp){
        label = li;
        t = ti;
        ot = t;
        best_t = t;
        e = ei;
        ddl = ddli - ei;
        r[TILE_TYPES::CLB] = clb;
        r[TILE_TYPES::RAM] = ram;
        r[TILE_TYPES::DSP] = dsp;
        credit = 0;
    };

    void reset() {
        t = ot;
        best_t = ot;
        credit = 0;
    }

    shared_ptr<string> to_string() {
        shared_ptr<string> re = std::make_shared<string>();
        re->append("(" + std::to_string(ot) + "," + std::to_string(e) + "," + std::to_string(ddl + e) + "," + std::to_string(best_t) + ",");
        re->append("(" + std::to_string(r[TILE_TYPES::CLB]) + "," + std::to_string(r[TILE_TYPES::RAM]) + "," + std::to_string(r[TILE_TYPES::DSP]) + "))");
        return re;
    }

    void set_area() {
        area = r[TILE_TYPES::CLB] + r[TILE_TYPES::RAM] + r[TILE_TYPES::DSP];
    }
};

typedef shared_ptr<Task> TaskPtr;

struct DDLOrder {
    bool operator()(const TaskPtr& p1, const TaskPtr& p2) {
        return p1->ddl > p2->ddl;
    }
};

typedef priority_queue<TaskPtr, vector<TaskPtr>, DDLOrder> TaskQueue;
//typedef queue<TaskPtr> TaskQueue;
typedef vector<TaskPtr> TaskPool;

void generate_tasks(Ctx &ctx, TaskQueue &tasks);
void select_tasks(Ctx& ctx, TaskQueue &tasks_pool, TaskPool &tasks_wait, int select);
int select_applied(TaskPool& src, vector<Sol>& sols);
void stop_tasks();