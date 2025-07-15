#include "ExhaustedSearch.h"
#include <random>

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(1, 100);

extern MetricFunc metric;
static int threads;
static int window;

// method1: reuse function in optimizers.h (have tested)
// drawback: is_legal re-check all positions while loop result can be suffix-sum
double ES_entry(Ctx &ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t) {
//    INFO("ES-p: Search placement for Task ", *t.to_string(), ".");
    long area = t.r[0] + t.r[1] + t.r[2];
    pair<long, long> p = BitmapOP::get_boundary();
    long xF = p.first, yF = p.second;
    vector<Sol> bests;
    double best_costs = 2.0;
    for (long x = 0; x < xF; x++){ // search xF
        for (long y = 0; y < yF; y++) { // search yF
            if (sc->at(x, y) == 1)
                continue;
            // tile (i, j) is free
            for (long h = 1; h + y - 1 < yF; h++) { // search h
                if (sc->at(x, y + h - 1) == 1) // pre-end
                    break;
                for (long w = ceil((double)area / h); x + w - 1 < xF; w++) {
                    Sol curr(x, y, w, h);
                    // return: 1-3:occupied(pre-end); 4-7:resource less(continue)
                    int flag = is_legal(ctx, curr, sc, t);
                    if (flag < 4 && flag > 0)
                        break;
                    if (flag == 0) {
                        double cost = metric(ctx, curr, t);
                        if (cost < best_costs) {
                            best_costs = cost;
                            bests.clear();
                            bests.push_back(curr);
//                            INFO("ES: A new optima searched out:", *curr.to_string(), " with best cost:", std::to_string(cost));
                        } else if (cost == best_costs) {
                            if (bests.size() < 10)
                                bests.push_back(curr);
//                            INFO("ES: Same best ", *curr.to_string(), " with best cost:", std::to_string(cost));
                        } // cost>best_costs, continue
                    }
                }
            }
        }
    }
    if (!bests.empty()) {
        memcpy(&s, &bests[0], sizeof(s));
//        INFO("ES: For TASK:", *t.to_string(), " best cost is:", std::to_string(best_costs), " with possible placement (TOP10):", std::to_string(bests.size()));
        string str;
        for (auto& best: bests)
            str += *best.to_string();
        INFO(str);
    }
    return best_costs;
}

long area = 0;
pair<long, long> boundaries;

void ES_p_work(Ctx& ctx, pair<long, long> range_x, pair<long, long> range_y, double& local_best_cost,
               vector<Sol>& local_bests, shared_ptr<Bitmap> sc, Task& t) {
    for (long x = range_x.first; x < range_x.second; x++) {
        for (long y = range_y.first; y < range_y.second; y++) {
            if (sc->at(x, y) == 1)
                continue;
            for (long h = 1; h + y - 1 < boundaries.second; h++) { // search h
                if (sc->at(x, y + h - 1) == 1) // pre-end
                    break;
                for (long w = ceil((double)area / h); x + w - 1 < boundaries.first; w++) {
                    Sol curr(x, y, w, h);
//                    if (t.t == 2)
//                        cout << endl;
                    // return: 1-3:occupied(pre-end); 4-7:resource less(continue)
                    int flag = is_legal(ctx, curr, sc, t);
                    if (flag < 4 && flag > 0)
                        break;
                    if (flag == 0) {
                        double cost = metric(ctx, curr, t);
                        if (cost < local_best_cost) {
                            local_best_cost = cost;
                            local_bests.clear();
                            local_bests.push_back(curr);
//                            INFO("ES: A new optima searched out:", *curr.to_string(), " with best cost:", std::to_string(cost));
                        } else if (cost == local_best_cost) {
                            local_bests.push_back(curr);
//                            INFO("ES: Same best ", *curr.to_string(), " with best cost:", std::to_string(cost));
                        } // cost>best_costs, continue
                    }
                }
            }
        }
    }
}

void ES_init(Ctx& ctx) {
    threads = stoi(ctx.param.at("es_threads"));
    window = stoi(ctx.param.at("window"));
}

double ES_entry_p(Ctx &ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t) {
//    key = key.substr(0, p);
    ThreadPool th(threads);
    INFO("ES-p: Search placement for Task ", *t.to_string(), " with threads ", std::to_string(threads));
    area = t.r[0] + t.r[1] + t.r[2];
    boundaries = BitmapOP::get_boundary();
    long groups = ceil(boundaries.first / 3);
    vector<vector<Sol>> local_bests(groups);
    vector<double> local_best_costs(groups, 2.0);
    for (long group = 0; group < groups; group++) {
        long start_x = group * 3;
        long end_x = min((group+1) * 3, boundaries.first);
        th.enqueue(ES_p_work, ref(ctx), pair<long, long>{start_x, end_x}, pair<long, long>{0, boundaries.second},
                   ref(local_best_costs[group]), ref(local_bests[group]), sc, ref(t));
    }
    th.wait();
    cout << "debug all threads done" << endl;

//    for (long assigned = 0; assigned < boundaries.first; assigned += 3) {
//        long end_x = min(boundaries.first, assigned + 3);
//        th.enqueue(ES_p_work, ref(ctx), pair<long, long>{assigned, end_x}, pair<long, long>{0, boundaries.second},
//                   ref(cost_in), ref(best_in), sc, ref(t));
//    }
//    th.wait();

//    vector<vector<Sol>> local_bests(threads);
//    vector<double> local_best_costs(threads, 2.0);
//    for (int worker = 0; worker < threads; worker++) {
//        long start_x = worker * seg;
//        long end_x = start_x + seg;
//        if (worker == threads - 1)
//            end_x = max(boundaries.first, end_x);
//        th.enqueue(ES_p_work, ref(ctx), pair<long, long>{start_x, end_x}, pair<long, long>{0, boundaries.second},
//                   ref(local_best_costs[worker]), ref(local_bests[worker]), sc, ref(t));
//    }
//    th.wait();

    int tag = -1;
    double global_best_cost = 2.0;
    for (int worker = 0; worker < local_bests.size(); worker++) {
        if (local_best_costs[worker] < global_best_cost) {
            tag = worker;
            global_best_cost = local_best_costs[worker];
        } else if (local_best_costs[worker] == global_best_cost) {
            for (auto& ele: local_bests[worker])
                local_bests[tag].push_back(ele);
        }
    }
//
//    for (int worker = 0; worker < threads; worker++) {
//        if (local_best_costs[worker] < global_best_cost) {
//            tag = worker;
//            global_best_cost = local_best_costs[worker];
//        }
//    }

    if (tag != - 1 && !local_bests[tag].empty()) {
        std::uniform_int_distribution<>::param_type disnew(0, local_bests[tag].size() - 1);
        dis.param(disnew);
        int seq = dis(gen);
        memcpy(&s, &local_bests[tag][seq], sizeof(s));
//        INFO("ES: For TASK:", *t.to_string(), " best cost is:", std::to_string(global_best_cost), " with possible placement:", std::to_string(local_bests[tag].size()));
        string str;
        for (auto& best: local_bests[tag])
            str += *best.to_string();
        INFO(str);
    }
    return global_best_cost;
}