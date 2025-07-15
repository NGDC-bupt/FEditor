#include "taskPool.h"
#include "log.h"
#include <memory>
#include <regex>
#include <random>
#include <limits>
#include <cmath>
using namespace std;

fstream fin;
static long Delta = 10;
static long cnt = 0;

void generate(Ctx &ctx, TaskQueue &tasks) {
    pair<long, long> p = BitmapOP::get_boundary();
    long xF = p.first;
    long yF = p.second;
    int task_speed = stoi(ctx.param["task_speed"]);
    int task_e = stoi(ctx.param["task_e"]);
    int clb = stoi(ctx.param["clb"]);
    int ram = stoi(ctx.param["ram"]);
    int dsp = stoi(ctx.param["dsp"]);
//    pair<long, long> p = get_pair();
    for (int i = 0; i < xF - yF; i++) {
        int n = rand() % (task_speed + 1);
        for (int j = 0; j < n; j++) {
            // t, e, clb, ram, dsp
            long e = rand() % task_e;
            long ddl = e + Delta;
            shared_ptr<Task> n = make_shared<Task>(cnt, xF + i, e, ddl, rand() % clb, rand() % ram, rand() % dsp);
            cnt++;
            n->set_area();
            tasks.push(n);
            log(LogType::info,"wow");
        }
    }
}

void load(TaskQueue &tasks) {
    string i = "";
    if(!getline(fin, i)){
        WARNING("test suite has been used up");
        return;
    }
    int nums = stoi(i);
    // (tin, e, ddl, (clb,ram,dsp), w, h, (clb,clb,clb,ram,dsp,dsp,clb))
    for (int j = 0; j < nums; j++) {
        getline(fin, i);

        regex re("\\d+");
        sregex_iterator begin(i.begin(), i.end(), re);
        sregex_iterator end;

        vector<long> tmp;
        int index = 0;
        for (sregex_iterator ii = begin; index < 6 && ii != end; ++ii, ++index) {
            const smatch& match = *ii;
            tmp.push_back((long)stoi(match.str()));
        }
        shared_ptr<Task> n = make_shared<Task>(cnt, tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5]);
        cnt++;
        n->set_area();
        tasks.push(n);
        INFO("new tasks: (", *(n->to_string()), ")");
    }
}


void generate_tasks(Ctx &ctx, TaskQueue &tasks) {
    if (ctx.param["test_suit"] == "0") {
        generate(ctx, tasks);
    } else {
        if (!fin.is_open()) {
            fin = fstream("./tasks/" + ctx.param["test_suit"] + ".txt", ios::in);
        }
        load(tasks);
    }
}

// select range relies on caller
void select_tasks(Ctx& ctx, TaskQueue &tasks_pool, TaskPool &tasks_wait, int select) {
    for (int i = 0; i < select; i++) {
        tasks_wait.push_back(tasks_pool.top());
        tasks_pool.pop();
        INFO("select tasks: (", *tasks_wait.back()->to_string(), ")");
    }
}

void stop_tasks() {
    if (fin.is_open()) {
       fin.close(); 
    }
    cnt = 0;
}

int select_applied(TaskPool& src, vector<Sol>& sols) {
    int n = src.size();
    if (n == 0)
        return 0;
    vector<int> time_masks(n, 0);
    vector<int> space_masks(n, 0);
    vector<int> masks(n, 0);
    for (int i = 0; i < src.size(); i++) {
        if (sols[i][0] == -1)
            continue;
        for (int j = i + 1; j < src.size(); j++) {  // check i, j whether they overlap
            if (sols[j][0] == -1)
                continue;
            if (src[i]->ot + src[i]->e > src[j]->ot &&
                src[j]->ot + src[j]->e > src[i]->ot) { // time overlap: !non-overlap
                time_masks[i] |= 1 << j;
                time_masks[j] |= 1 << i;
            }
            if ((sols[i][0] < sols[j][0] + sols[j][2]) && (sols[j][0] < sols[i][0] + sols[i][2]) &&
                (sols[i][1] < sols[j][1] + sols[j][3]) &&
                (sols[j][1] < sols[i][1] + sols[i][3])) { // space overlap: overlap for projection in x,y axis;
                space_masks[i] |= 1 << j;
                space_masks[j] |= 1 << i;
            }
            masks[i] = time_masks[i] & space_masks[i];
            masks[j] = time_masks[j] & space_masks[j];
        }
    }


    const double INF = std::numeric_limits<double>::lowest();;
    vector<double> dp(1 << n, INF);
    dp[0] = 0;
    for (int mask = 0; mask < (1 << n); ++mask) {
        if (dp[mask] == INF) continue;
        for (int j = 0; j < n; ++j) {
            if ((mask & (1 << j)) != 0) continue; // j has been chosen
            if ((mask & masks[j]) != 0) continue; // j overlap with i
            if (sols[j][0] == -1) continue; // reject cannot choose
            int new_mask = mask | (1 << j);
            dp[new_mask] = max(dp[new_mask], dp[mask] +  (1 - src[j]->credit));
        }
    }
    // find the maximal resource usage combination  OR  credit
    return (int)(max_element(dp.begin(), dp.end()) - dp.begin());
}