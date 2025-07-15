#pragma once
#include <map>
#include <memory>
#include <vector>
#include <utility>
#include <string>
#include "bitmap.h"
#include <mutex>
using namespace std;

enum Optimizer {
    SA, BO, PSO
};

const int TILE_TYPE = 3;
enum TILE_TYPES {
    CLB, RAM, DSP
};

class Sol {
public:
    long value[4]{};
    Sol () {
        value[0] = 0;
        value[1] = 0;
        value[2] = 0;
        value[3] = 0;
    }
    Sol(long a, long b, long c, long d) {
        value[0] = a;
        value[1] = b;
        value[2] = c;
        value[3] = d;
    }

    long& operator[](int index) {
        return value[index];
    };

    shared_ptr<string> to_string(){
        shared_ptr<string> re = make_shared<string>();
        re->append("(" + std::to_string(value[0])+",");
        re->append(std::to_string(value[1])+",");
        re->append(std::to_string(value[2])+",");
        re->append(std::to_string(value[3])+")");
        return re;
    }


};

// long[4]: [xF, yF, w, h]
//using Sol = long[4];
//class Sol {
//public:
//    long value[4];
//    Sol () {
//        value[0] = 0;
//        value[1] = 0;
//        value[2] = 0;
//        value[3] = 0;
//    }
//    Sol(long a, long b, long c, long d) {
//        value[0] = a;
//        value[1] = b;
//        value[2] = c;
//        value[3] = d;
//    }
//
//    long& operator[](int index) {
//        return value[index];
//    };
//};

// bitmap:allocation; f:help calculate
// s_i
class Diagram{
public:
    shared_ptr<Bitmap> states;
    //vector<shared_ptr<Island>> islands;
    double utility;
    int players;

    Diagram() : utility(0.0), players(1) {
        //states = shared_ptr<Bitmap>(new Bitmap());
        states = make_shared<Bitmap>();
        //islands.push_back(shared_ptr<Island>(new Island(BitmapOP::get_boundary())));
    };

    Diagram(Diagram& object) : utility(0.0), players(1) {
        // = shared_ptr<Bitmap>(new Bitmap(object.states));
        states = make_shared<Bitmap>(object.states);
        //for (auto& island : object.islands)
        //    islands.push_back(shared_ptr<Island>(new Island(island)));
    }

    // Diagram(DiagramPtr& object) : utility(0.0), players(1) {
    //     states = shared_ptr<Bitmap>(new Bitmap(object->states));
    //     for (int i = 0; i < object->islands.size(); i++)
    //         islands.push_back(shared_ptr<Island>(new Island(object->islands[i])));
    // }

    shared_ptr<string> to_string() {
        //shared_ptr<string> re = shared_ptr<string>(new string());
        shared_ptr<string> re = make_shared<string>();
//        re->append(*states->to_string());
        //for (auto& island : islands)
            //re->append(*island->to_string());
        return states->to_string();
    }

    void apply(shared_ptr<Bitmap> sol, long wastes) {
        this->states->pile_up_with_wastes(sol, wastes);
        // Consider: no need for island ?
    }

    double get_utilization() {
        return states->get_utilization();
    }

    double get_wastes() {
        return states->get_wastes();
    }

    // Diagram* copy(){
    //     Diagram* d = new Diagram();
    //     if (v != NULL) {
    //         d->v = v->copy();
    //     }
    //     for (auto i = f.begin(); i != f.end(); i++) {
    //         d->f.push_back(new Island(**i));
    //     }
    //     d->utility = utility;
    //     d->players = players;
    //     return d;
    // }

//    double collect_distribute(double du) {
//        utility += du;
//        players++;
//        return (utility/players);
//    }
//
//    double leaving(double ou) {
//        utility = utility - ou;
//        players--;
//        return (utility / players);
//    }
//
//    double try_collect(double du) {
//        return (utility + du) / (players + 1);
//    }
};

// [(t_i,s_i)]
typedef shared_ptr<Diagram> DiagramPtr;
typedef std::map<long, DiagramPtr>::iterator DiagramIter;

// Context: environment
class Ctx{
public:
    shared_ptr<vector<int>> fpga[TILE_TYPE];// (xF,yF) in segment function
    map<long, DiagramPtr> diagrams;// [(t_m,s_m),...,(t_n,s_n)]
    map<string, string> param;
    long long time;
    map<long, long long> task_time;
    mutex time_lock;
    // multi-thread
    pair<DiagramIter, DiagramIter> seq;

    Ctx() {
        for (auto & i : fpga) {
            i = make_shared<vector<int>>();
            i->push_back(0);
        }
        time = 0;
    };

    Ctx(const Ctx& other) {
        fpga[0] = other.fpga[0];
        fpga[1] = other.fpga[1];
        fpga[2] = other.fpga[2];
        for (auto& kv: other.param)
            param[kv.first] = kv.second;
        time = 0; // no matter
        // new t_m/t_n <- thread2 -> last_diagram <- thread1 -> new t_m/t_n
        for (auto& kv: other.diagrams)
            diagrams[kv.first] = kv.second;
    }

    // multi-thread safe
    void add_time(long long duration, long task_label) {
        time_lock.lock();
        time += duration;
        auto tar = task_time.find(task_label);
        if (tar == task_time.end())
            task_time[task_label] = duration;
        else
            tar->second += duration;
        time_lock.unlock();
    }
    void construct() {
        diagrams[0] = std::make_shared<Diagram>();
    }
    shared_ptr<string> to_diagrams_string() {
        shared_ptr<string> re = make_shared<string>();
        string s = "     \t";
        for (int i = 1; i < fpga[0]->size(); i++) {
            if (fpga[TILE_TYPES::CLB]->at(i) - fpga[TILE_TYPES::CLB]->at(i - 1) != 0) {
                s += "l\t";
            } else if (fpga[TILE_TYPES::RAM]->at(i) - fpga[TILE_TYPES::RAM]->at(i - 1) != 0) {
                s += "r\t";
            } else {
                s += "d\t";
            }
        }
        s += "\n\n";
        for (auto& diagram: diagrams)
            re->append("Time-" + std::to_string(diagram.first) +" Wastes:" + std::to_string(diagram.second->states->wastes) + "\n"+ *diagram.second->to_string() + s);
        return re;
    }
    void get_near_resource(long& x, int type) {
        int cur = fpga[type]->at(x), ddx = 0;
        for (int dx = 1; x - dx > 0; dx++) {
            if (fpga[type]->at(x - dx) != cur) {
                x = x - dx;
                ddx = dx;
                break;
            }
        }
        for (int dx = 1; x + ddx + dx < fpga[type]->size(); dx++) {
            if (fpga[type]->at(x + ddx + dx) != cur) {
                if (dx < ddx)
                    x = x + ddx + dx;
                break;
            }
        }
    }
};