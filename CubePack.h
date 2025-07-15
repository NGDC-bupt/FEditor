#pragma once
#include"module.h"
#include "taskPool.h"
#include "optimizers/optimizers.h"
#include "evaluation.h"

namespace CubePack {
    int apply(Ctx &ctx, Sol &sol, Task& t);

// relaxation on sc
    double alloc(Ctx &ctx, TaskPtr t, Sol &s, long relaxation);
// relaxation on cost
//    void alloc2(Ctx& ctx, shared_ptr<Task> t, Sol &s, long relaxation);
    void alloc_part(Ctx &ctx, TaskPtr& t, Sol &sol, DiagramIter& _t_m, DiagramIter& _t_n);
    void alloc_para(Ctx &ctx, TaskPool& tasks, vector<Sol> &sols, long relaxation);

    void alloc2_in(Ctx &ctx, shared_ptr<Task> t, Sol &sol, long relaxation);

// int multi_alloc(Ctx &ctx, TaskPool &tasks, vector<Sol> &sols);

//Bitmap* get_s_c(Ctx &ctx, Task &t);
}
