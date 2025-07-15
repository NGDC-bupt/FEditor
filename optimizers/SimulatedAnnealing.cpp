#include <cmath>
#include <set>
#include <thread>
#include <random>
#include "SimulatedAnnealing.h"
using namespace std;

extern MetricFunc metric;

// non changeable
static double T, T_min, alpha;
static int max_iter;
static long boundary_x, boundary_y;
static bool first = false;
static mutex best_lock;
static int STABLE;

// changeable
//static set<string> tabu_table;
static Sol best = {-1,-1,-1,-1};
static double best_cost = 2.0;

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<> xrandom(0,1);
static std::uniform_int_distribution<> yrandom(0,1);
static std::uniform_int_distribution<> step(-5,5);
static std::uniform_int_distribution<> delta(1,5);
static std::uniform_real_distribution<> exploring(0.0, 1.0);


static int LIMIT = 100;

inline void justify(Ctx& ctx, Sol &current, int flag, long area, shared_ptr<Bitmap> sc) {
    int dv = delta(gen);
    if (flag == 1) { // other convex
        get_corner_but(current, sc);
    } else if (flag == 2) { // move down
        current[1] -= dv;
    } else if (flag == 3) { // move left
        current[0] -= dv;
    } else if(flag == 4) { // not enough, increase h
        current[3] += dv;
    } else if(flag == 5 || flag == 6 || flag == 7) { // not include
        move_to_resource(ctx, current, flag - 5);
    }
}


// generate new solution
static bool random_modify(Ctx& ctx, Sol &current, long area, shared_ptr<Bitmap> sc, Task& t) {
    for (int i = 0; i < 4; i++) // move by step
        current[i] += step(gen);

    pre_legal(current, area, boundary_x, boundary_y);
    int cnt = 0, flag = is_legal(ctx, current, sc, t);
    int cnt2 = 0;
    while (flag!=0 && cnt < LIMIT) {
        justify(ctx, current, flag, area, sc);
        pre_legal(current, area, boundary_x, boundary_y);
        flag = is_legal(ctx, current, sc, t);
//        if (tabu_table.find(*current.to_string()) == tabu_table.end())
            cnt++;
        cnt2++;
        if (cnt2 == 2*LIMIT) {
            current[0] = xrandom(gen);
            current[1] = yrandom(gen);
            current[3] = xrandom(gen);
            current[4] = yrandom(gen);
        }
        if (cnt2 == 10 * LIMIT) {
            cnt = LIMIT;
        }
    }
    return cnt == LIMIT;
}

void SA_init(Ctx& ctx) {
    // changeable ones
    best_cost = 2.0;
    best = {-1,-1,-1,-1};
//    tabu_table.clear();
    // unchangeable ones
    if (!first) {
        pair<long, long> boundaries = BitmapOP::get_boundary();
        boundary_x = boundaries.first;
        boundary_y = boundaries.second;
        // [0, xF - 1] -> [0, xF)
        std::uniform_int_distribution<>::param_type xnew(0, boundary_x - 1);
        // [0, yF - 1] -> [0, yF)
        std::uniform_int_distribution<>::param_type ynew(0, boundary_y - 1);
        xrandom.param(xnew);
        yrandom.param(ynew);
        T = stod(ctx.param.at("T"));
        T_min = stod(ctx.param.at("T_min"));
        alpha = stod(ctx.param.at("alpha_opt"));
        max_iter = stoi(ctx.param.at("max_iter"));
        STABLE = stoi(ctx.param.at("stable"));
        first = true;
    }
}

int sa(Ctx &ctx, long area, shared_ptr<Bitmap> sc, Task& t){
    Sol current = {0, 0, 0, 0};
    current[3] = yrandom(gen);
    current[2] = ceil((double)area/current[3]);
    pre_legal(current, area, boundary_x, boundary_y);
    int cnt = 0, flag = is_legal(ctx, current, sc, t);
    while (flag!=0 && cnt < LIMIT * 10) {
        justify(ctx, current, flag, area, sc);
        pre_legal(current, area, boundary_x, boundary_y);
        flag = is_legal(ctx, current, sc, t);
//        if (tabu_table.find(*current.to_string()) == tabu_table.end())
            cnt++;
    }
    if (cnt == LIMIT * 10) {
        ERROR("SA: cannot find any feasible solution");
        return -1;
    }

    double current_cost = metric(ctx, current, t);
    double current_best = current_cost;
//    tabu_table.insert(*current.to_string());
    best_lock.lock();
    best_cost = current_best;
    memcpy(&best, &current, sizeof(Sol));
    best_lock.unlock();
    // Simulated annealing main loop.
    double T0 = T;
    int decreasing = 0;
    int stable = STABLE;
    while (T0 > T_min) {
//        INFO("New Temperature begin:", current_cost, " of solution:", *current.to_string());
        for ( int iter = 0; iter < max_iter && stable > 0; iter++){
            Sol next;
            memcpy(&next, &current, sizeof(Sol));
            bool limit = random_modify(ctx, next, area, sc, t);
            if (limit)
                continue;
            double new_cost = metric(ctx, next, t);
//            tabu_table.insert(*current.to_string());
//            INFO("New cost:", new_cost, " of solution:", *next.to_string());

            if (new_cost < current_cost) {
//                INFO("Replace current. Origin cost:", current_cost, " of solution ", *current.to_string());
                memcpy(&current, &next, sizeof(Sol));
                current_cost = new_cost;
                stable = STABLE;
                // Update the best solution.
                if (new_cost < current_best) {
                    current_best = new_cost;
                    best_lock.lock();
                    if (new_cost < best_cost) {
//                    INFO("Replace new best. Origin cost:", best_cost, " of solution ", *best.to_string());
                        memcpy(&best, &next, sizeof(Sol));
                        best_cost = new_cost;
                    }
                    best_lock.unlock();
                }
            } else {
                // The probability of accepting a worse solution.
                double p = exp(-(new_cost - current_cost) / T); // (0, 1]
                double r = exploring(gen);
                if (r < p) {
//                    INFO("Explore to replace current. Origin cost:", current_cost, " of solution ", *current.to_string());
                    memcpy(&current, &next, sizeof(Sol));
                    current_cost = new_cost;
                }
            }
            stable--;
        }
        T0 *= alpha;
//        INFO("Temperature decrease to ", T);
        if (decreasing % 20 == 0)
            INFO("Temperature at: ", T0, " end with cost: ", current_cost, " of solution: ", *current.to_string());
        decreasing++;
    }
    return 0;
}

double SA_entry(Ctx &ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t){\
    best_cost = 2.0;
    best = {-1,-1,-1,-1};
    long area = t.area;
    if (sa(ctx, area, sc, t) != -1)
        memcpy(&s, &best, sizeof(Sol));
    return best_cost;
}

double SA_entry_p(Ctx &ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t){
    best_cost = 2.0;
    best = {-1,-1,-1,-1};
    long area = t.area;
    int threads = stoi(ctx.param["threads"]);

    ThreadPool pool(threads);

    for (int i = 0; i < threads; i++)
        pool.enqueue(sa, ref(ctx), area, sc, ref(t));
    
    pool.wait();
    memcpy(&s, &best, sizeof(Sol));
    return best_cost;
}