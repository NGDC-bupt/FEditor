#ifndef BITMAP_H
#define BITMAP_H

#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <memory>
#include <iostream>
using namespace std;

namespace BitmapOP {
    std::pair<long, long> get_boundary();
    void set_boundary(long x, long y);
}

class Bitmap{
public:
    char* v;// each byte for one cell
    long m, n;//camera offset
    long wastes;
    bool father;

    Bitmap(bool f = true);

    Bitmap(long ax, long ay, long aw, long ah, bool f = true);

    Bitmap(shared_ptr<Bitmap>& ptr);

    ~Bitmap();
    void alloc(long ax, long ay, long aw, long ah);
    void dealloc(long ax, long ay, long aw, long ah);
    static bool is_out_of_fpga(long ax, long ay, long aw, long ah);
    static long get_total();
    int at(long xi, long yi);
    double get_utilization();
    double get_wastes();
    double quality();
    shared_ptr<Bitmap> copy();
    shared_ptr<Bitmap> shallow_copy();
    shared_ptr<Bitmap> create_view(int _m, int _n);
    void change_view(int _m, int _n);
    void pile_up(shared_ptr<Bitmap> b);
    void pile_up_no_fresh(shared_ptr<Bitmap> b);
    void pile_up_with_wastes(shared_ptr<Bitmap> b, long w);
    int L0Norm();
    void or_op(shared_ptr<Bitmap> b);
    void and_op(shared_ptr<Bitmap> b);
    int cnt_op();
    int corner();
    shared_ptr<string> to_string();
    void turn_edge();
    void solid_boundary();
    void dfs(int x, int y, char label, char* visited);
    void fresh();
    pair<long, long> get_corner_but(long x, long y);
    void get_MER(long& xi, long& yi, long& wi, long& hi);
    // transparent functions
    long size_x();
    long size_y();
    char real_at(long di, long dj);
    shared_ptr<string> to_real_string();
};


#endif //BITMAP_H