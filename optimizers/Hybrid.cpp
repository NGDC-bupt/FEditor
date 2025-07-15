#include <cmath>
#include <set>
#include <thread>
#include <random>
#include "Hybrid.h"
using namespace std;

extern MetricFunc metric;

static double T, T_min, alpha;
static int max_iter;
static long boundary_x, boundary_y;
static bool first = false;
static int STABLE, particles;

static double INERTIA, COGNITIVE, SOCIAL, V_MAX;

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<> xrandom(0,1);
static std::uniform_int_distribution<> yrandom(0,1);
static std::uniform_int_distribution<> vrandom(0,1);
static std::uniform_int_distribution<> step(-5,5);
static std::uniform_int_distribution<> delta(1,5);
static std::uniform_real_distribution<> exploring(0.0, 1.0);
static std::uniform_int_distribution<> zeroone(0,1);

struct Particle {
    Sol position;
    Sol velocity;
    Sol local_best;
    double local_best_cost;

    Particle(Sol& p) {
        position = p;
        local_best = position;
        velocity = {0,0,0,0};
        local_best_cost = 2.0;
    }
};

static int LIMIT = 100;

inline void justify(Ctx& ctx, Sol &current, int flag, long area, shared_ptr<Bitmap> sc) {
    int dv = delta(gen);
    if (flag == 1) {
        get_corner_but(current, sc);
    } else if (flag == 2) {
        current[1] -= dv;
    } else if (flag == 3) {
        current[0] -= dv;
    } else if(flag == 4) {
        current[3] += dv;
    } else if(flag == 5 || flag == 6 || flag == 7) {
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
        cnt++;
        cnt2++;
        if (cnt2 == 2*LIMIT) {
            current[0] = xrandom(gen);
            current[1] = yrandom(gen);
            current[2] = xrandom(gen);
            current[3] = yrandom(gen);
        }
        if (cnt2 == 10 * LIMIT) {
            cnt = LIMIT;
        }
    }
    return cnt == LIMIT;
}

void Hybrid_init(Ctx& ctx) {
    if (!first) {
        pair<long, long> boundaries = BitmapOP::get_boundary();
        boundary_x = boundaries.first;
        boundary_y = boundaries.second;
        // [0, xF - 1] -> [0, xF)
        std::uniform_int_distribution<>::param_type xnew(0, boundary_x - 1);
        // [0, yF - 1] -> [0, yF)
        std::uniform_int_distribution<>::param_type ynew(0, boundary_y - 1);
        std::uniform_int_distribution<>::param_type vnew(-V_MAX, V_MAX);
        xrandom.param(xnew);
        yrandom.param(ynew);
        vrandom.param(vnew);
        T = stod(ctx.param.at("T"));
        T_min = stod(ctx.param.at("T_min"));
        alpha = stod(ctx.param.at("alpha_opt"));
        max_iter = stoi(ctx.param.at("max_iter"));
        STABLE = stoi(ctx.param.at("stable"));
        particles = stoi(ctx.param.at("particles"));
        first = true;
    }
}

inline void update_velocity(Particle& p, Sol& global_best) {
    for (int d = 0; d < 4; d++) {
        double r1 = zeroone(gen);
        double r2 = zeroone(gen);
        p.velocity[d] = INERTIA * p.velocity[d] + COGNITIVE * r1 * (p.local_best[d] - p.position[d]) + SOCIAL * r2 * (global_best[d] - p.position[d]);
        if (p.velocity[d] > V_MAX) p.velocity[d] = V_MAX;
        if (p.velocity[d] < -V_MAX) p.velocity[d] = -V_MAX;
    }
}

inline bool update_position(Ctx& ctx, Particle& p, shared_ptr<Bitmap> sc, Task& t, long area) {
    for (int d = 0; d < 4; d++)
        p.position[d] += (long)p.velocity[d];
    pre_legal(p.position, area, boundary_x, boundary_y);
    int cnt = 0, flag = is_legal(ctx, p.position, sc, t);
    int cnt2 = 0;
    while (flag!=0 && cnt < LIMIT) {
        justify(ctx, p.position, flag, area,  sc);
        pre_legal(p.position, area, boundary_x, boundary_y);
        flag = is_legal(ctx, p.position, sc, t);
        cnt++;
        cnt2++;
        if (cnt2 == 2*LIMIT) {
            p.position[0] = xrandom(gen);
            p.position[1] = yrandom(gen);
            p.position[3] = xrandom(gen);
            p.position[4] = yrandom(gen);
        }
        if (cnt2 == 10 * LIMIT) {
            cnt = LIMIT;
        }
    }
    return cnt == LIMIT;
}

void iterator_once(Ctx& ctx, Particle& p, shared_ptr<Bitmap> sc, Task& t, long area, Sol global_best) {
    if (update_position(ctx, p, sc, t, area))
        return;
    update_velocity(p, global_best);
    double new_cost = metric(ctx, p.position, t);
    // Update personal best
    if (new_cost < p.local_best_cost) {
//        INFO("Particle ", *p.position.to_string() ," update local best:", new_cost, " original:", p.local_best_cost);
        p.local_best = p.position;
        p.local_best_cost = new_cost;
    }
}

int sa_outer(Ctx &ctx, long area, shared_ptr<Bitmap> sc, Task& t, Sol& best, double& best_cost){
    Sol current = {0, 0, 0, 0};
    current[3] = yrandom(gen);
    current[2] = ceil((double)area/current[3]);
    pre_legal(current, area, boundary_x, boundary_y);
    int cnt = 0, flag = is_legal(ctx, current, sc, t);
    while (flag != 0 && cnt < LIMIT * 10) {
        justify(ctx, current, flag, area, sc);
        pre_legal(current, area, boundary_x, boundary_y);
        flag = is_legal(ctx, current, sc, t);
        if (flag == 0)
            flag = is_legal(ctx, current, sc, t);
        cnt++;
    }
    if (cnt == 10 * LIMIT) {
//        ERROR("SA: cannot find any feasible solution");
        return -1;
    }
    int threads = stoi(ctx.param["threads"]);
    ThreadPool thp(threads);

    double current_cost = metric(ctx, current, t);
    best_cost = current_cost;
    memcpy(&best, &current, sizeof(Sol));
    double T0 = T;
    int decreasing = 0;
    int stable = STABLE; // current cost stay stable
    while (T0 > T_min) {
        // PSO
        vector<Particle> ps(particles, Particle(current));
//        for (auto& p: ps)
//            update_velocity( p, current);
        for (auto& p: ps)
            thp.enqueue(update_velocity, ref(p), current);
        thp.wait();
        for ( int iter = 0; iter < max_iter && stable > 0; iter++){
            // iter_once
            for (auto& p: ps)
                thp.enqueue(iterator_once, ref(ctx), ref(p), sc, ref(t), area, current);
            thp.wait();
//            for (auto& p: ps)
//                iterator_once(ctx, p, sc, t, area, current);
            auto best_p = max_element(ps.begin (), ps.end (), [](const Particle& a, const Particle& b) {
                return a.local_best_cost > b.local_best_cost; // cost minimal
            });

            if (best_p->local_best_cost < best_cost) {
                memcpy(&current, &best_p->position, sizeof(Sol));
                current_cost = best_p->local_best_cost;
                stable = STABLE;
                // Update the best solution.
                if (best_p->local_best_cost < best_cost) {
                    memcpy(&best, &best_p->position, sizeof(Sol));
                    best_cost = best_p->local_best_cost;
                }
            } else {
                double p = exp(-(best_p->local_best_cost - current_cost) / T); // (0, 1]
                double r = exploring(gen);
                if (r < p) {
                    memcpy(&current, &best_p->position, sizeof(Sol));
                    current_cost = best_p->local_best_cost;
                }
            }
            stable--;
        }
        T0 *= alpha;
//        if (decreasing % 20 == 0)
//            INFO("Temperature at: ", T0, " end with cost: ", current_cost, " of solution: ", *current.to_string());
        decreasing++;
    }
    return 0;
}

double Hybrid_entry(Ctx &ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t){
    long area = t.area;
    double best_cost;
    Sol best;
    if (sa_outer(ctx, area, sc, t, best, best_cost) != -1)
        memcpy(&s, &best, sizeof(Sol));
    return best_cost;
}

//double Hybrid_entry_p(Ctx &ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t){
//    long area = t.area;
//    double best_cost;
//    Sol best;
//    if (sa_outer(ctx, area, sc, t, best, best_cost) != -1)
//        memcpy(&s, &best, sizeof(Sol));
//    return best_cost;
//}

double Hybrid_entry_p(Ctx &ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t){
    long area = t.area;
    int threads = stoi(ctx.param["sa_threads"]);
    vector<double> sa_best_costs(threads, 2.0);
    vector<Sol> sa_bests(threads, {-1,-1,-1,-1});

    ThreadPool pool(threads, "outer");

//    cout << "debug begin outer threads" << endl;
    for (int i = 0; i < threads; i++)
        pool.enqueue(sa_outer, ref(ctx), area, sc, ref(t), ref(sa_bests[i]), ref(sa_best_costs[i]));
    pool.wait();
//    cout << "debug all outer threads done" << endl;

    sa_outer(ctx, area, sc, t, sa_bests[0], sa_best_costs[0]);

    auto pos = max_element(sa_best_costs.begin(), sa_best_costs.end(), [](const double& a, const double& b) {
        return a > b;
    }) - sa_best_costs.begin();

    memcpy(&s, &sa_bests[pos], sizeof(Sol));
    return sa_best_costs[pos];
}