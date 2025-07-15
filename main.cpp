#include "CubePack.h"
#include "taskPool.h"
#include "log.h"
#include "evaluation.h"
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <cmath>

using namespace std;

static map<string, TILE_TYPES> TILE_TYPE_NAME = {
        {"CLB", TILE_TYPES::CLB},
        {"RAM", TILE_TYPES::RAM},
        {"DSP", TILE_TYPES::DSP}
};

inline bool get_kv(ifstream& fin, string& key, string& value) {
    string s;
    if(!getline(fin, s))
        return false;
    int pos = (int)s.find(':');
    key = s.substr(0, pos); 
    value = s.substr(pos + 1);
    return true;
}

// construct env params
void env_construct(Ctx& ctx, string& file) {
    // env.config
    init_log();
    ifstream fin = ifstream("./config/" + file +".config", ios::in);
    if (!fin) {
        ERROR("open file wrong :./config/", file, ".config");
    }
    string key, value;

    while (get_kv(fin, key, value))
        ctx.param[key] = value;
    fin.close();
    // if (Optimizer[ctx.param["optimizer"]] == NULL) {
    //     LOG::ERROR("LOG::ERROR: cannot find optimizer" + ctx.param["optimizer"]);
    //     return -1;
    // }

    // if (ctx.param["place_mode"] != "single" && ctx.param["place_mode"]!= "multi") {
    //     LOG::ERROR("LOG::ERROR: cannot find place_mode" + ctx.param["place_mode"]);
    //     return -1;
    // }
    // return 0;
}

// construct fpga logic view (resource function)
void fpga_construct(Ctx& ctx, string& file) {
    // fpga.config
    // record yF
    vector<int> cur(TILE_TYPE, 0);
    ifstream fin("./config/" + file +".config");
    if (!fin) {
        ERROR("open file wrong :./config/", file, ".config");
    }
    string key, value;

    get_kv(fin, key, value);
    long xF = stoi(value);
    get_kv(fin, key, value);
    long yF = stoi(value);
    BitmapOP::set_boundary(xF, yF);
    get_kv(fin, key, value);
    fin.close();
    string c;
    for(int i = 0; i < value.size(); i++) {
        if (value[i] == 'l')
            c = "CLB";
        else if (value[i] == 'd')
            c = "DSP";
        else if(value[i] == 'r')
            c = "RAM";
        cur[TILE_TYPE_NAME[c]]++;
        ctx.fpga[TILE_TYPES::CLB]->push_back(cur[TILE_TYPES::CLB]);
        ctx.fpga[TILE_TYPES::DSP]->push_back(cur[TILE_TYPES::DSP]);
        ctx.fpga[TILE_TYPES::RAM]->push_back(cur[TILE_TYPES::RAM]);
    }
}

int task_construct(Ctx& ctx) {
    // tasks.config
    ifstream fin = ifstream("./config/tasks.config", ios::in);
    string key, value;

    while (get_kv(fin, key, value))
        ctx.param[key] = value;
    fin.close();
    return 0;
}

struct Params {
    string test_suit;
    string alpha;
    string beta;
    string gamma;

    Params(string& t, string& a, string& b, string& c) {
        test_suit = t;
        alpha = a;
        beta = b;
        gamma = c;
    }
};

int main_body(shared_ptr<Params> params) {
    Ctx ctx = Ctx();
    string env_file = "env";

    // command params, LOG::ERROR handling inside
    env_construct(ctx, env_file);
    // re construct
    ctx.param["test_suit"] = params->test_suit;
    ctx.param["alpha"] = params->alpha;
    ctx.param["beta"] = params->beta;
    ctx.param["gamma"] = params->gamma;

    string fpga_file = "fpga";
    if (ctx.param.find("fpga") != ctx.param.end())
        fpga_file = ctx.param.at("fpga");
    fpga_construct(ctx, fpga_file);
    task_construct(ctx);
    optimizer_construct(ctx);
    ctx.construct();
    Evaluation::init();


    TaskQueue task_pool;
    int time_iteration = stoi(ctx.param["iteration"]);
    int select = stoi(ctx.param["select"]);
    int relaxation = stol(ctx.param["relaxation"]);
    for(int time = 0; time < time_iteration; time++) {
        generate_tasks(ctx, task_pool);
        if (task_pool.empty()) {
            INFO("Done! WoW!");
            break;
        }

        // place: lcp, lcpp (cpm reserved)
        // all placements will consider ddl
        if (ctx.param["mode"] == "cp") { // sequential with(>0) or(=0) without relaxation
            while (!task_pool.empty()) {
                Sol sol{-1,-1,-1,-1};
                auto& task = task_pool.top();
                CubePack::alloc2_in(ctx, task, sol, relaxation);
//                cout << "debug" << "out of alloc2_in" << endl;
                if (sol[0] == -1) { // best_cost == 2.0
                    log(LogType::info,"reject:", *task->to_string());
                    Evaluation::reject();
                } else {
                    log(LogType::info,"accept:", *task->to_string(), " with solution:", *sol.to_string(), " and cost:", task->credit);
                    Evaluation::accept();
                    Evaluation::ACC(task->ot, task->e);
                    CubePack::apply(ctx, sol, *task);
                    INFO("\n", *ctx.to_diagrams_string());
                    fstream solprint = fstream("solutions.txt", ios::out | ios::app);
                    solprint << *task->to_string() << "with" << *sol.to_string() << endl;
                    solprint.close();
                }
                task_pool.pop();
            }
        } else { // cp-p
            while (!task_pool.empty()) { // even though we place multiple tasks at 1 iteration, we need to place all tasks in this period
                int nums = min(select, (int)task_pool.size());
                vector<Sol> best_sols(nums, {-1,-1,-1,-1});
                TaskPool selected_tasks;
                select_tasks(ctx, task_pool, selected_tasks, nums);
                for (int i = 0; i < nums; i++) {
                    CubePack::alloc2_in(ctx, selected_tasks[i], best_sols[i], relaxation);
                }
//                ThreadPool thp(select); // select agents
//                for (int i = 0; i < nums; i++) {
//                    thp.enqueue(CubePack::alloc2_in, ref(ctx), selected_tasks[i], ref(best_sols[i]), relaxation);
//                }
//                thp.wait();
                for (int i = 0, delta = 0; i < nums; i++) {
                    if (best_sols[i][0] == -1) { // reject
                        log(LogType::info,"reject:", *selected_tasks[i - delta]->to_string());
                        Evaluation::reject();
                        selected_tasks.erase(selected_tasks.begin() + (i - delta));
                        best_sols.erase(best_sols.begin() + (i - delta));
                        delta++;
                        nums--;
                    }
                }
                // apply the best or non-overlap
                vector<int> returning;
                if (!selected_tasks.empty()) {
                    auto max_index = max_element(selected_tasks.begin(), selected_tasks.end(), [](const shared_ptr<Task> a, const shared_ptr<Task> b) {
                        return a->credit > b->credit;   }) - selected_tasks.begin();
                    log(LogType::info,"accept:", *selected_tasks[max_index]->to_string(), " with solution:", *best_sols[max_index].to_string(), " and cost:", selected_tasks[max_index]->credit);
                    Evaluation::accept();
                    CubePack::apply(ctx, best_sols[max_index], *selected_tasks[max_index]);
                    for (int i = 0; i < selected_tasks.size();i++){
                        if (i == max_index)
                            continue;
                        else {
                            selected_tasks[i]->reset();
                            task_pool.push(selected_tasks[i]);
                            returning.push_back(i);
                        }
                    }
                }
//                int applied = select_applied(selected_tasks, best_sols);
//                for (int i = 0; i < nums; i++) {
//                    int chose = 1 & applied;
//                    if (chose == 1) { // applied
//                        log(LogType::info,"accept:", *selected_tasks[i]->to_string(), " with solution:", *best_sols[i].to_string(), " and cost:", selected_tasks[i]->credit);
//                        Evaluation::accept();
//                        CubePack::apply(ctx, best_sols[i], *selected_tasks[i]);
//                    } else { // return to queue
//                        selected_tasks[i]->reset();
//                        task_pool.push(selected_tasks[i]);
//                        returning.push_back(i);
//                    }
//                   applied >>= 1;
//                }
                for (auto& index: returning)
                    INFO("Task ", *selected_tasks[index]->to_string(), "delayed cause rearranging");
                INFO("\n", *ctx.to_diagrams_string());
            }
        }
        // } else if (ctx.param["place_mode"] == "multi") {
        //     multi_alloc(ctx, tasks_wait, sols);
        //     for (int i = 0; i < tasks_wait.size(); i++) {
        //         LOG::INFO("Solution generated: (", sols[i][0], ",", sols[i][1], ",", sols[i][2], ",", sols[i][3], ")");
        //     }
        // }
//        for (auto& ele: tasks_wait) {
//            ele->credit = 0.0;
//            task_pool.push(ele);
//            WARNING("Task:", *ele->to_string(), " goes back to pool");
//        }
    }

//    fstream fouting = fstream("dia.txt", ios::out | ios::trunc);
//    fouting << *ctx.diagrams.rbegin()->second->to_string() << endl;
//    fouting.close();
    Evaluation::result(ctx);
//    Evaluation::time_trend(ctx);
//    logFile(LogType::info);
//    INFO("Optimizer:", ctx.param["optimizer"]);
//    INFO("Task suit:", ctx.param["test_suit"]);
//    INFO("Mode:", ctx.param["mode"]);
//    INFO("Relaxation:", ctx.param["relaxation"]);
//    INFO("Metric:", ctx.param["alpha"], ",", ctx.param["beta"],",",ctx.param["gamma"]);
//    INFO("Last task ends at:", ctx.diagrams.rbegin()->first);
//    INFO("Processing Time consuming:", std::to_string((double)ctx.time/1e6));
//    INFO("Utilization:", Evaluation::utilization(ctx), "        Acceptance Rate (",Evaluation::acc(), "/", Evaluation::all(),"):", Evaluation::acceptance(),"        Waste Rate:", Evaluation::wastes(ctx));
    stop_tasks();
    return 0;
}

int main(int argc, char* argv[]) {
    auto ctx = make_shared<Ctx>();
    string env_file = "env";
    if (argc > 1)
        env_file = argv[1];

    // command params, LOG::ERROR handling inside
    env_construct(*ctx, env_file);


    auto params = make_shared<Params>(ctx->param["test_suit"], ctx->param["alpha"], ctx->param["beta"], ctx->param["gamma"]);

    int start_a = (int)(std::stod(ctx->param["alpha"]) * 10);
    int start_b = (int)(std::stod(ctx->param["beta"]) * 10);

    int step = (int)(std::stod(ctx->param["automatic_step"]));
    int start_t = (int)(std::stod(ctx->param["task_seq"])) - 1;
    string automatic_set = ctx->param.at("automatic_set");
    string automatic_metric = ctx->param.at("automatic_metric");
    string automatic_type = ctx->param.at("automatic_type");
    ctx = nullptr;

    if (automatic_set == "true") {
        for (int i = start_t; i < 10; i++) {
            string task_set = "-0" + std::to_string(i + 1);
            if (i == 9)
                task_set = "-10";
            params->test_suit = automatic_type + task_set;
            if (automatic_metric == "true") {

                for (int a = start_a; a <= 10; a += step) {
                    if (a != start_a)
                        start_b = 0;
                    for (int b = start_b; b <= 10 - a; b += step) {
                        int c = 10 - a - b;
                        params->alpha = "0." + std::to_string(a);
                        params->beta = "0." + std::to_string(b);
                        params->gamma = "0." + std::to_string(c);
//                        main_body(params);
                    }
                }
            } else {
                main_body(params);
            }
        }
    } else if (automatic_metric == "true") {
        for (int a = start_a; a <= 10; a += step) {
            if (a != start_a)
                start_b = 0;
            for (int b = start_b; b <= 10 - a; b += step) {
                int c = 10 - a - b;
                params->alpha = std::to_string((double)a / 10);
                params->beta = std::to_string((double)b / 10);
                params->gamma = std::to_string((double)c / 10);
                main_body(params);
            }
        }
    } else {
        main_body(params);
    }
}