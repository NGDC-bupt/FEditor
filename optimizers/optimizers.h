#pragma once
#include "../module.h"
#include "../log.h"
#include "../threadpool.h"
#include "../taskPool.h"

typedef double (*OptimizerFunc)(Ctx &ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t);
typedef double (*MetricFunc)(Ctx &ctx, Sol& s, Task& t);
typedef void (*InitParamFunc)(Ctx& ctx);

void register_optimizer(string name, OptimizerFunc func);

//enum Metric{ ABC, AB, AC, BC, A, B, C};

void optimizer_construct(Ctx &ctx);

// ctx, tasks, sol, s_c
double optimize(Ctx &ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t);
//int multi_optimize(Ctx &ctx, Sol &s);

void get_corner_but(Sol& cur, shared_ptr<Bitmap> sc);
void move_to_resource(Ctx& ctx, Sol& s, int type);

void pre_legal(Sol& current, long area, long boundary_x, long boundary_y);

// legality check
int is_legal(Ctx& ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t);

// 3 metrics
double single_metric(Ctx &ctx, Sol& s, Task& t);
double multi_metric(Ctx &ctx, Sol& s, Task& t);
// a_i=(ra-rk)/(3*f)
double internal_frag(const Ctx &ctx, Sol& s, Task& t);
// b_i
double external_frag(Sol& s, shared_ptr<Bitmap> is);
// c_i
double compactness(Sol& s, shared_ptr<Bitmap> sm);
//long shoelace_formula(shared_ptr<vector<shared_ptr<VertexNode>>> land);
// // c_i
// double external_frag1(vector<shared_ptr<Island>>& lands);
// double external_frag2(vector<shared_ptr<Island>>& lands);
// double external_frag3(vector<shared_ptr<Island>>& lands);