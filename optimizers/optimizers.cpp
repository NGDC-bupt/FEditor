#include "optimizers.h"
#include "SimulatedAnnealing.h"
#include "ParticleSwarmOptimization.h"
#include "Hybrid.h"
//#include "GeneticAlgorithm.h"
#include "ExhaustedSearch.h"
#include <climits>
#include <cmath>
#include <fstream>
#include <memory>
using namespace std;

// multi thread secure
shared_ptr<Bitmap> edge;

map<string, OptimizerFunc> Optimizer;
map<string, InitParamFunc> Initors;
MetricFunc metric;

// param from env.config
double alpha_a, beta_b, gamma_c;

void register_optimizer(string name, OptimizerFunc func){
    Optimizer[name] = func;
}

void read_config(Ctx &ctx, string config) {
    fstream fin = fstream(config.c_str(), ios::in);
    string str = "";
    while (getline(fin, str)) {
        string key, value;
        int pos = str.find(':');
        key = str.substr(0, pos);
        value = str.substr(pos + 1);
        ctx.param[key] = value;
    }
    fin.close();
}

//true: single false:multi
void optimizer_construct(Ctx& ctx) {
    // env.config
    alpha_a = stod(ctx.param["alpha"]);
    beta_b = stod(ctx.param["beta"]);
    gamma_c = stod(ctx.param["gamma"]);
    // optimizer.config
    string s = ctx.param["optimizer"];
    s = s.substr(0, s.find('_'));
    read_config(ctx, "./config/" + s + ".config");
    register_optimizer("SA", SA_entry);
    register_optimizer("SA_p", SA_entry_p);
    register_optimizer("ES", ES_entry);
    register_optimizer("ES_p", ES_entry_p);
    register_optimizer("PSO", PSO_entry);
    register_optimizer("PSO_p", PSO_entry_p);
    register_optimizer("Hybrid", Hybrid_entry);
    register_optimizer("Hybrid_p", Hybrid_entry_p);
    //TODO: check
//    register_optimizer("GA", GA_entry);
//    register_optimizer("GA_p", GA_entry_p);
    Initors["SA"] = SA_init;
    Initors["ES"] = ES_init;
    Initors["PSO"] = PSO_init;
    Initors["Hybrid"] = Hybrid_init;
    Initors.at(s)(ctx);
    if (ctx.param["mode"] != "cpm")
        metric = single_metric;
    else //(ctx.param["mode"] == "multi")
        metric = multi_metric;

    shared_ptr<Bitmap> _edge = std::make_shared<Bitmap>();
    _edge->turn_edge();
    edge = _edge;
}

double optimize(Ctx &ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t) {
    return Optimizer.at(ctx.param.at("optimizer"))(ctx, s, sc, t);
}

void get_corner_but(Sol& cur, shared_ptr<Bitmap> sc) {
    pair<long, long> fv = sc->get_corner_but(cur[0], cur[1]);
    cur[0] = fv.first;
    cur[1] = fv.second;
}

void move_to_resource(Ctx& ctx, Sol& cur, int type) {
    ctx.get_near_resource(cur[0], type);
}

// 1. non-negative
// 2. w,h neq 0
// 3. in the FPGA
void pre_legal(Sol& current, long area, long boundary_x, long boundary_y) {
    // 1. x,y,w,h positive
    for (int i = 0; i < 4; i ++)
        if (current[i] < 0)
            current[i] = - current[i];

    // 2. w,h neq 0
    if (current[3] == 0)
        current[3] = 1;
    // coarse grained: area has to satisfy requirements
    current[2] = max(current[2], (long)ceil(area/current[3]));
    // 3. in fpga
    if(current[0] + current[2] - 1 >= boundary_x)
        current[0] = boundary_x - current[2] - 1;
    if(current[0] < 0) {
        current[2] += current[0];
        current[0] = 0;
    }

    if(current[1] + current[3] - 1 >= boundary_y)
        current[1] = boundary_y - current[3] - 1;
    if(current[1] < 0){
        current[3] += current[1];
        current[1] = 0;
    }
}

// legality check
// require optimizers to check:
// 1. non-negative
// 2. w,h neq 0
// 3. in the FPGA
int is_legal(Ctx& ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t){
    // constraint2: f_r(xF,yF,w,h) = 0
    for (int i = s[0]; i < s[0] + s[2]; i++)
        for (int j = s[1]; j < s[1] + s[3]; j++)
            if (sc->at(i, j) != 0) {
                if (i == s[0] && j == s[1])
                    return 1; // other vertex
                else if (j != s[1])
                    return 2; // move down
                else if (j == s[1])
                    return 3; // move left
            }

    // constraint3: h*(f(xF+w)-f(xF)) >= r_k
    int i = 0;
    for (; i < TILE_TYPE; i++) {
        long own = s[3] * (ctx.fpga[i]->at(s[0] + s[2]) - ctx.fpga[i]->at(s[0]));
        if (own < t.r[i]){
            if (own != 0)
                return 4; // not enough
            else
                return 5 + i; // not include -> occupy 5 6 7
        }
    }

    return 0;
}

// 3 metrics
double single_metric(Ctx &ctx, Sol &s, Task& t) {
    double res = 0.0, internal = 0.0;
    if (alpha_a > 0)
        internal = alpha_a * internal_frag(ctx, s, t);
//    if (s[0] == 0 && s[1] == 0 && s[2] == 20 && s[3] == 5)
//        cout << endl;
    // (\alpha*a_i + \beta*b_i + \gamma*c_i) * (t_m+1 - t_m) / (t_n - t_m)
    for (auto c = ctx.seq.first; c != ctx.seq.second; c++) {
        double m = 0.0;
        m += internal;
        if (beta_b > 0)
            m += beta_b * external_frag(s, c->second->states);
        if (gamma_c > 0)
            m += gamma_c * compactness(s, c->second->states);
        res += m * (next(c)->first - c->first);
    }
    return res / t.e;
}

double multi_metric(Ctx &ctx, Sol& s, Task& t) {
    return 0.0;
}

// a_i = \frac{1}{3}\sum\frac{r_{alloc}^{type}-r_k^{type}}{r_{alloc}^{type}}
double internal_frag(const Ctx &ctx, Sol& s, Task& t) {
    double res = 0;
    for (int i = 0; i < TILE_TYPE; i++) {
        long alloc = s[3] * (ctx.fpga[i]->at(s[0] + s[2]) - ctx.fpga[i]->at(s[0]));
        if (alloc != 0)
            res += (double)(alloc - t.r[i]) / alloc;
    }
    return res / 3;
}

// b_i = \sum_{islands}(\frac{area_{island}}{\sum_{islands}area_{island}}\cdot\frac{area_{island}}{minRecArea_{island}})
double external_frag(Sol& s, shared_ptr<Bitmap> is) {
    map<char, long> cnt;
    map<char, long*> bounding; // minx, miny, maxx, maxy
    shared_ptr<Bitmap> tmp = make_shared<Bitmap>(is);
    tmp->alloc(s[0], s[1], s[2], s[3]);
    // is->alloc(s);
    for (long i = 0; i < tmp->size_x(); i++) {
        for (long j = 0; j < tmp->size_y(); j++) {
            char label = tmp->real_at(i, j);
            if (label == (char)0)
                continue;
            cnt[label] += 1;
            if (bounding.find(label) == bounding.end()) { // new label
                bounding[label] = new long[4];
                bounding[label][0] = LONG_MAX;
                bounding[label][1] = LONG_MAX;
                bounding[label][2] = -1;
                bounding[label][3] = -1;
            }
            bounding[label][0] = bounding[label][0] < i ? bounding[label][0] : i;
            bounding[label][1] = bounding[label][1] < j ? bounding[label][1] : j;
            bounding[label][2] = bounding[label][2] > i ? bounding[label][2] : i;
            bounding[label][3] = bounding[label][3] > j ? bounding[label][3] : j;
        }
    }
    double term1 = 0.0, term2 = 0.0;
    for (auto& island: cnt) {
        term1 += (double)island.second;
        char label = island.first;
        term2 += (double)island.second * island.second / ((bounding[label][2] - bounding[label][0] + 1) * (bounding[label][3] - bounding[label][1] + 1));
        delete[] bounding[label];
        bounding[label] = nullptr;
    }
    if (cnt.empty())
        return 0;
    // is->dealloc(s);

    // metric 0: allocation
    return 1.0 - term2 / term1;
}

// alloc, sm:Am matrix, edge: Aedge matrix, total: 2xFyF+xF+yF
// c_i = 1 - \frac{||(Alloc_k^'\wedge A_m) \vee (Alloc_k\wedge A_{edge})||_0}{2x_Fy_F+x_F+y_F}
double compactness(Sol& s, shared_ptr<Bitmap> sm) {
//    if (s[0] == 0 && s[1] == 0 && s[2] == 20 && s[3] == 2)
//        cout << endl;
    // metric0: placement
//    long total = 2 * s[2] + 2 * s[3];
//    long total = edge->get_total();
    auto p = BitmapOP::get_boundary();
    long total = ((p.first + 1) * (p.second + 1) * s[2] * s[3]) / (p.first * p.second);
    // metric1: FPGA whole
    Bitmap alloc = Bitmap(s[0], s[1], s[2], s[3]);

    shared_ptr<Bitmap> allock = alloc.copy();
    // alloc_right
    shared_ptr<Bitmap> alloc_ = alloc.create_view(-1, 0);
    allock->or_op(alloc_);
    // alloc_left
    alloc_->change_view(2, 0);
    allock->or_op(alloc_);
    // alloc_up
    alloc_->change_view(-1, -1);
    allock->or_op(alloc_);
    // alloc_down
    alloc_->change_view(0, 2);
    allock->or_op(alloc_);
    allock->and_op(sm);
    int w_1 = allock->L0Norm();

    alloc.and_op(edge);
//    int w_2 = alloc.L0Norm();
    int w_2 = alloc.L0Norm() + alloc.corner();
    return 1.0 - ((double)(w_2 + w_1) / (total));
}



// // c_i=1-\frac{1}{\sum minRec}\sum area_{island}
// double external_frag1(vector<shared_ptr<Island>>& lands) {
//     long area = 0;
//     long min_rec = 0;
//     for (auto land: lands) {
//         int vs = land->size();
//         long area_v = shoelace_formula(*land);
//         long min_rec = min_bouding_rec(*land);
//     }
//     return (double)(1 - area / min_rec);
// }

// // b_i= \frac{1}{\sum_{num(island)}1}
// double external_frag2(vector<shared_ptr<Island>>& lands) {
//     return 1.0 / (double)lands.size();
// }

// // b_i = 1 - \prod[frac{4}{V_i}\times\frac{A_i}{A_{free}}]
// double external_frag3(vector<shared_ptr<Island>>& lands) {
//     double res = 0.0;
//     long free = 0;
//     for (auto& land : lands) {
//         long area_v = shoelace_formula(*land);
//         // must be an integer
//         free += (long)(abs(area_v) / 2);
//         res += (double)(area_v * 4 / land->size());
//     }
//     for (int i = 0; i < lands.size(); i++)
//         res /= (double)free;
//     return 1 - res;
// }

// // according to land(vector of vertex), calculate the area
// long shoelace_formula(Island& land) {
//     int vs = land.size();
//     long area_v = 0;
//     for (int i = 0; i < vs; i++) {
//         auto v1 = land[i];
//         auto v2 = land[(i + 1) % vs];
//         area_v += (v1.first - v2.first) * (v1.second - v2.second);
//     }
//     return area_v;
// }

// according to land(vector of vertex), calculate the area
//long shoelace_formula(const  shared_ptr<vector<shared_ptr<VertexNode>>> land) {
//    int vs = land->size();
//    long area_v = 0;
//    for (int i = 0; i < vs; i++) {
//        auto v1 = land->at(i);
//        auto v2 = land->at((i + 1) % vs);
//        area_v += (v1->x - v2->x) * (v1->y - v2->y);
//    }
//    return area_v;
//}

// double cross(const Point& O, const Point& A, const Point& B) {
//     return (A.first - O.first) * (B.second - O.second) - (A.second - O.second) * (B.first - O.first);
// }

// Island* convex_hull(Island& points) {
//     int n = points.size(), k = 0;
//     if (n <= 3) return NULL;
//     Island* hull_p = new Island(2 * n);
//     Island& hull = *hull_p;

//     sort(points.begin(), points.end(), [](const Point& a, const Point& b) {
//         return a.first < b.second || (a.first == b.first && a.second < b.second);
//     });

//     for (int i = 0; i < n; ++i) {
//         while (k >= 2 && cross(hull[k - 2], hull[k - 1], points[i]) <= 0) k--;
//         hull[k++] = points[i];
//     }

//     for (int i = n - 1, t = k + 1; i > 0; --i) {
//         while (k >= t && cross(hull[k - 2], hull[k - 1], points[i - 1]) <= 0) k--;
//         hull[k++] = points[i - 1];
//     }

//     hull.resize(k - 1);
//     return hull_p;
// }

// //最小lin
// double rotating_calipers(const Island& hull) {
//     int n = hull.size();
//     if (n < 3) return 0;

//     double minArea = numeric_limits<double>::max();

//     for (int i = 0, k = 1; i < n; ++i) {
//         int j = (i + 1) % n;
//         while (cross(hull[i], hull[j], hull[(k + 1) % n]) > cross(hull[i], hull[j], hull[k])) {
//             k = (k + 1) % n;
//         }

//         double dx = hull[j].first - hull[i].first;
//         double dy = hull[j].second - hull[i].second;
//         double length = sqrt(dx * dx + dy * dy);
//         double width = abs((hull[k].first - hull[i].first) * dy - (hull[k].second - hull[i].second) * dx) / length;

//         minArea = min(minArea, length * width);
//     }

//     return minArea;
// }

// long min_bouding_rec(Island& land) {
//     // get convex hull of the land
//     Island* hull = convex_hull(land);
//     // apply rotate caliper algorithm
//     long rec_area = rotating_calipers(*hull);
//     delete hull;
//     return rec_area;
// }

// double multi_metric(Ctx &ctx, Task &t, Sol &s) {
//     double sal = 0.0;
//     for (auto &it = t_m; it != t_m; it++){
//         double ui = single_metric(ctx, t, s);
//         sal += it->second->try_collect(ui);
//     }
//     return sal;
// }