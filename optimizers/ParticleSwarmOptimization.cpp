#include "ParticleSwarmOptimization.h"
#include <vector>
#include <random>
#include <cmath>
#include <thread>
using namespace std;

extern MetricFunc metric;

static int LIMIT = 100;
static long boundary_x, boundary_y;
static int threads = 1;
// The data structure of a particle in the PSO algorithm.
struct ParticlePSO {
    Sol position;
    Sol velocity;
    Sol best_position;
    double best_cost;

    ParticlePSO() {
        position = {0,0,0,0};
        velocity = {0,0,0,0};
        best_position = {0,0,0,0};
        best_cost = 2.0;
    }

    ParticlePSO(Sol& other, double m) {
        position = other;
        velocity = {0,0,0,0};
        best_position = other;
        best_cost = m;
    }

    shared_ptr<string> to_string() {
        return position.to_string();
    }
};

static Sol global_best = {-1,-1,-1,-1};
static double global_best_cost = 2.0;
static mutex gb_lock;

static int PARTICLE_COUNT, MAX_ITER;
static double INERTIA, COGNITIVE, SOCIAL, V_MAX;
static int STABLE;

static bool first = false;

static Sol MER;

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<> xrandom(0,1);
static std::uniform_int_distribution<> yrandom(0,1);
static std::uniform_int_distribution<> vrandom(0,0);
static std::uniform_int_distribution<> delta(1,5);
static std::uniform_int_distribution<> zeroone(0,1);

void PSO_init(Ctx& ctx) {
    if (!first) {
        PARTICLE_COUNT =stoi(ctx.param.at("PARTICLE_COUNT"));
        MAX_ITER = stoi(ctx.param.at("MAX_ITER"));
        INERTIA = stod(ctx.param.at("INERTIA"));
        COGNITIVE = stod(ctx.param.at("COGNITIVE"));
        SOCIAL = stod(ctx.param.at("SOCIAL"));
        V_MAX = stod(ctx.param.at("V_MAX"));
        STABLE = stoi(ctx.param.at("STABLE"));
        auto p = BitmapOP::get_boundary();
        boundary_x = p.first;
        boundary_y = p.second;
        // [0, xF - 1] -> [0, xF)
        std::uniform_int_distribution<>::param_type xnew(1, boundary_x);
        // [0, yF - 1] -> [0, yF)
        std::uniform_int_distribution<>::param_type ynew(1, boundary_y);
        std::uniform_int_distribution<>::param_type vnew(-V_MAX, V_MAX);
        xrandom.param(xnew);
        yrandom.param(ynew);
        vrandom.param(vnew);
        first = true;
    }
    global_best = {-1,-1,-1,-1};
    global_best_cost = 2.0;
}

inline void justify(Ctx& ctx, Sol &current, int flag, shared_ptr<Bitmap> sc) {
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

bool init(Ctx& ctx, ParticlePSO& p, shared_ptr<Bitmap> sc, Task& t) {
    long area = t.area;
    p.position={0,0,0,0};
    p.position[3] = yrandom(gen);
    p.position[2] = ceil((double)area/p.position[3]);
    pre_legal(p.position, area, boundary_x, boundary_y);
    int cnt = 0, flag = is_legal(ctx, p.position, sc, t);
    while (flag!=0 && cnt < LIMIT * 10) {
        justify(ctx, p.position, flag, sc);
        pre_legal(p.position, area, boundary_x, boundary_y);
        flag = is_legal(ctx, p.position, sc, t);
        cnt++;
    }
    if (cnt == LIMIT * 10) {
//        ERROR("PSO: cannot find any feasible solution");
        return false;
    }

    // Randomly initialize velocity (all four dimensions are updated, values range [-V_MAX,V_MAX])
    for (int i = 0; i < 4; i++)
        p.velocity[i] = vrandom(gen);

    p.best_cost = metric(ctx, p.position, t);

    gb_lock.lock();
    if (p.best_cost < global_best_cost) {
        global_best_cost = p.best_cost;
        memcpy(&global_best, &p.position, sizeof(Sol));
    }
    gb_lock.unlock();
    return true;
}

inline void update_velocity(ParticlePSO& p) {
    for (int d = 0; d < 4; d++) {
        double r1 = zeroone(gen);
        double r2 = zeroone(gen);
        p.velocity[d] = INERTIA * p.velocity[d] + COGNITIVE * r1 * (p.best_position[d] - p.position[d]) + SOCIAL * r2 * (global_best[d] - p.position[d]);
        if (p.velocity[d] > V_MAX) p.velocity[d] = V_MAX;
        if (p.velocity[d] < -V_MAX) p.velocity[d] = -V_MAX;
    }
}

inline bool update_position(Ctx& ctx, ParticlePSO& p, shared_ptr<Bitmap> sc, Task& t) {
    long area = t.area;
    for (int d = 0; d < 4; d++)
        p.position[d] += (long)p.velocity[d];

    pre_legal(p.position, area, boundary_x, boundary_y);
    int cnt = 0, flag = is_legal(ctx, p.position, sc, t);
    int cnt2 = 0;
    while (flag!=0 && cnt < LIMIT) {
        justify(ctx, p.position, flag, sc);
        pre_legal(p.position, area, boundary_x, boundary_y);
        flag = is_legal(ctx, p.position, sc, t);
//        if (tabu_table.find(*current.to_string()) == tabu_table.end())
        cnt++;
        cnt2++;
        if (cnt2 == 2 * LIMIT) {
            p.position[0] = xrandom(gen) - 1;
            p.position[1] = yrandom(gen) - 1;
            p.position[3] = xrandom(gen);
            p.position[4] = yrandom(gen);
        }
        if (cnt2 == 10 * LIMIT) {
            cnt = LIMIT;
        }
    }
    return cnt == LIMIT;
}

void iterator_once(Ctx& ctx, ParticlePSO& p, shared_ptr<Bitmap> sc, Task& t) {
    if (update_position(ctx, p, sc, t))
        return;
    update_velocity(p);

    double new_cost = metric(ctx, p.position, t);

    // Update personal best
    if (new_cost < p.best_cost) {
//        INFO("Particle ", *p.to_string() ," update local best:", new_cost, " original:", p.best_cost);
        for (int d = 0; d < 4; d++) {
            p.best_position[d] = p.position[d];
        }
        p.best_cost = new_cost;
    }

    // Update global best -> return to father caller to get rid of lock waiting
    gb_lock.lock();
    if (new_cost < global_best_cost) {
//        INFO("Update global best:", new_cost, "of ", *p.to_string()," original:", global_best_cost, " of placement (", global_best[0], ",", global_best[1], ",", global_best[2], ",", global_best[3],")");
        for (int d = 0; d < 4; d++) {
            global_best[d] = p.position[d];
        }
        global_best_cost = new_cost;
    }
    gb_lock.unlock();
}

double PSO_entry(Ctx &ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t) {
    global_best = {-1,-1,-1,-1};
    global_best_cost = 2.0;
    auto swarm = make_shared<vector<ParticlePSO>>(PARTICLE_COUNT, ParticlePSO());
    vector<int> failures;
    for (int i = 0; i < swarm->size(); i++) {
        if (!init(ctx, swarm->at(i), sc, t))
            failures.push_back(i);
    }
    int deltadd = 0;
    for (auto& f: failures)
        swarm->erase(swarm->begin() + (f-deltadd));
//    sc->get_MER(MER[0], MER[1], MER[2], MER[3]);
//    if (is_legal(ctx, MER, sc, t) == 0) {
//        swarm->push_back(ParticlePSO(MER,metric(ctx, MER, t)));
//        if (swarm->rbegin()->best_cost < global_best_cost) {
//            global_best_cost = swarm->rbegin()->best_cost;
//            memcpy(&global_best, &swarm->rbegin()->position, sizeof(Sol));
//        }
//    }
    if (swarm->empty())
        return 0;
//    WARNING("Particles remain ", swarm->size());

    // begin to iterate
    for (int iter = 0; iter < MAX_ITER; iter++)
        for (auto& p: *swarm)
            iterator_once(ctx, p, sc, t);

    memcpy(&s, &global_best, sizeof(Sol));
    return global_best_cost;
}

double PSO_entry_p(Ctx &ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t){
    global_best = {-1,-1,-1,-1};
    global_best_cost = 2.0;
    threads = stoi(ctx.param["threads"]);
    ThreadPool pool = ThreadPool(threads);
    auto* swarm = new vector<ParticlePSO>(PARTICLE_COUNT);

    for (auto& p: *swarm) {
        pool.enqueue(init, ref(ctx), ref(p), sc, ref(t));
    }
    pool.wait();
//    cout << "DEBUG" << "donw waitting for init" << endl;

//    sc->get_MER(MER[0], MER[1], MER[2], MER[3]);
//    if (is_legal(ctx, MER, sc, t) == 0) {
//        swarm->push_back(ParticlePSO(MER,metric(ctx, MER, t)));
//        if (swarm->rbegin()->best_cost < global_best_cost) {
//            global_best_cost = swarm->rbegin()->best_cost;
//            memcpy(&global_best, &swarm->rbegin()->position, sizeof(Sol));
//        }
//    }

    for (int iter = 0, stable = 0; iter < MAX_ITER && stable < STABLE; iter++) {
        double gbc = global_best_cost;
        for (auto& p: *swarm) {
           pool.enqueue(iterator_once, ref(ctx), ref(p), sc, ref(t));
        }
        pool.wait();
//        cout << "DEBUG" << "interator once" << endl;
        if (gbc == global_best_cost) {
            stable++;
        } else {
            stable = 0;
        }
    }
//    cout << "DEBUG" << "iterator all" << endl;

    memcpy(&s, &global_best, sizeof(Sol));
    delete swarm;
    return global_best_cost;
}