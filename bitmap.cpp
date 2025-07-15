#include "bitmap.h"
#include <memory>
#include <string>
#include <vector>

static long xF, yF;
static long offset, cells;

// REMAINDER: to simplify, we take 0 as occupied, n > 0 as island seq
// however, we let outside as 0 as idle, 1 as occupied

std::pair<long, long> BitmapOP::get_boundary() {
    return std::pair<long, long>({xF, yF});
}

void BitmapOP::set_boundary(long x, long y) {
    xF = x;
    yF = y;
    offset = xF + 2;
    cells = (xF+2) * (yF+2);
}

Bitmap::Bitmap(bool f): m(1), n(1), father(f), wastes(0){
    if (f) {
        v = (char*)malloc(cells * sizeof(char));
        memset(v, (char)1, cells * sizeof(char));
    }
}

Bitmap::Bitmap(long ax, long ay, long aw, long ah, bool f): m(1), n(1), father(f), wastes(0){
    if (f){
        v = (char*)malloc(cells * sizeof(char));
        memset(v, (char)1, cells * sizeof(char));
    }
    alloc(ax, ay, aw, ah);
}

Bitmap::Bitmap(shared_ptr<Bitmap>& ptr) {
    m = ptr->m;
    n = ptr->n;
    father = true;
    wastes = ptr->wastes;
    v = (char*)malloc(cells * sizeof(char));
    memcpy(v, ptr->v, cells * sizeof(char));
}

Bitmap::~Bitmap(){
    if (father)
        free(v);
}

void Bitmap::alloc(long ax, long ay, long aw, long ah) {
    for (long i = ax + m; i < ax + aw + m; i++)
        for (long j = ay + n; j < ay + ah + n; j++)
            v[offset * j + i] = 0;
    fresh();
}

void Bitmap::dealloc(long ax, long ay, long aw, long ah) {
    for (long i = ax + m; i < ax + aw + m; i++)
        for (long j = ay + n; j < ay + ah + n; j++)
            v[offset * j + i] = 1; // any but 0
    fresh();
}

bool Bitmap::is_out_of_fpga(long ax, long ay, long aw, long ah) {
    // since aw ah cannot be 0, we can only check x+w and y+h
    return ax < 0 || ay < 0 || ax + aw-1 >= xF || ay + ah-1 >= yF;
}

long Bitmap::get_total(){
    return 2 * xF * yF + xF + yF;
}

int Bitmap::at(long xi, long yi){
    // true(1): occupied
    return v[(xi + m) + offset * (yi + n)] == 0;
}

double Bitmap::get_utilization() {
    return (double)(cnt_op() - wastes) / (xF * yF);
}

double Bitmap::get_wastes() {
    return (double)(wastes) / (xF * yF);
}

double Bitmap::quality() {
    return 0.0;
}

shared_ptr<Bitmap> Bitmap::copy(){
    shared_ptr<Bitmap> b = make_shared<Bitmap>();
    b->m = m;
    b->n = n;
    memcpy(b->v, v, cells * sizeof(char));
    return b;
}

shared_ptr<Bitmap> Bitmap::shallow_copy() {
    shared_ptr<Bitmap> b = make_shared<Bitmap>();
    b->m = m;
    b->n = n;
    b->v = v;
    // move mangement of v
    b->father = true;
    father = false;
    return b;
}

shared_ptr<Bitmap> Bitmap::create_view(int _m, int _n){
    shared_ptr<Bitmap> b = make_shared<Bitmap>(false);
    b->v = v;
    b->m = m + _m;
    b->n = n + _n;
    return b;
}

void Bitmap::change_view(int _m, int _n){
    m += _m;
    n += _n;
}

void Bitmap::pile_up(shared_ptr<Bitmap> b){
    or_op(b);
}

void Bitmap::pile_up_no_fresh(shared_ptr<Bitmap> b) {
//    cout << "debug in pile up no fresh" << endl;
    for (int i = 0; i < xF; i++)
        for (int j = 0; j < yF; j++)
            // one 0 should be 0
            v[(i + m) + offset * (j + n)] &= b->real_at(i ,j);
//    cout << "debug out pile up no fresh" << endl;
}

void Bitmap::pile_up_with_wastes(shared_ptr<Bitmap> b, long w) {
    wastes += w;
    or_op(b);
}


int Bitmap::L0Norm(){
    return cnt_op();
}

void Bitmap::or_op(shared_ptr<Bitmap> b){
    for (int i = 0; i < xF; i++)
        for (int j = 0; j < yF; j++)
            // one 0 should be 0
            v[(i + m) + offset * (j + n)] &= b->real_at(i ,j);
//    cout << "debug out of for in or_op of Bitmap" << endl;
    fresh();
//    cout << "debug out of fresh in or_op of Bitmap" << endl;
}

void Bitmap::and_op(shared_ptr<Bitmap> b){
    for (int i = 0; i < xF; i++)
        for (int j = 0; j < yF; j++)
            // two 0 should be 0
            v[(i + m) + offset * (j + n)] += b->real_at(i ,j);
    fresh();
}

int Bitmap::cnt_op(){
    int cnt = 0;
    for (int i = m; i < xF + m; i++)
        for (int j = n; j < yF + n; j++)
            cnt += v[offset * j + i] == 0;
    return cnt;
}

int Bitmap::corner() {
    int cnt = 0;
    if (v[offset * (n) + m] == 0)
        cnt++;
    if (v[offset * (n + yF - 1) + m] == 0)
        cnt++;
    if (v[offset * (n) + m + xF - 1] == 0)
        cnt++;
    if (v[offset * (n + yF - 1) + m + xF - 1] == 0)
        cnt++;
    return cnt;
}

//    void reverse(long xi, long yi, long wi, long hi){
//        for (int i = xi; i < xi + wi; i++)
//            for (int j = yi; j < yi + hi; j++)
//                v[m + i][n + j] = 0;
//    }

shared_ptr<string> Bitmap::to_string() {
    shared_ptr<string> re = std::make_shared<string>();
    for (int j = yF + n - 1; j >= n; j--) {
        re->append("row" + std::to_string(j - n) + ":\t");
        for (int i = m; i < xF + m; i++) {
            re->push_back((char) ((int) '0' + (int) (v[offset * j + i] == 0)));
            re->push_back('\t');
        }
        re->push_back('\n');
    }
    re->append("     \t");
    for (int i = 0; i < xF; i++){
        re->append(std::to_string(i)+"\t");
    }
    re->push_back('\n');
    return re;
}

void Bitmap::get_MER(long& xi, long& yi, long& wi, long& hi) {
    vector<int> left(yF, 0);
    vector<int> right(yF, yF);
    std::vector<int> height(yF, 0);

    int maxArea = 0;
    int x = 0, y = 0, w = 0, h = 0;

    for (int i = 0; i < xF; ++i) {
        int curLeft = 0, curRight = yF;

        for (int j = 0; j < yF; ++j) {
            if (v[i + m + offset * (n + j)] == 1) {
                height[j]++;
            } else {
                height[j] = 0;
            }
        }

        for (int j = 0; j < yF; ++j) {
            if (v[i + m + offset * (n + j)] == 1) {
                left[j] = std::max(left[j], curLeft);
            } else {
                left[j] = 0;
                curLeft = j + 1;
            }
        }

        for (int j = yF - 1; j >= 0; --j) {
            if (v[i + m + offset * (n + j)] == 1) {
                right[j] = std::min(right[j], curRight);
            } else {
                right[j] = yF;
                curRight = j;
            }
        }

        for (int j = 0; j < yF; ++j) {
            int area = (right[j] - left[j]) * height[j];
            if (area > maxArea) {
                maxArea = area;
                x = left[j];
                y = i - height[j] + 1;
                w = right[j] - left[j];
                h = height[j];
            }
        }
    }
    xi = x;
    yi = y;
    wi = h;
    hi = w;
}

// specific functions

void Bitmap::turn_edge(){
    if (!father) {
        // cout << "LOG::ERROR: turn_edge() called on a non-father Bitmap" << endl;
        return;
    }
    memset(v, (char)1, cells * sizeof(char));
    for (int i = m; i < m + xF; i++) {
        v[offset * (yF + n - 1) + i] = 0;
        v[offset * n + i] = 0;
    }
    for (int j = n; j < n + yF; j++) {
        v[offset * j + (m + xF - 1)] = 0;
        v[offset * j + m] = 0;
    }
}

void Bitmap::solid_boundary() {
    for (int i = m - 1; i < xF + 2; i++) {
        v[offset * (yF + n) + i] = 0;
        v[offset * (n - 1) + i] = 0;
    }
    for (int j = n - 1; j < yF + 2; j++) {
        v[offset * j + (m + xF)] = 0;
        v[offset * j + m - 1] = 0;
    }
}

const int dx[4] = {-1, 1, 0, 0};
const int dy[4] = {0, 0, -1, 1};

void Bitmap::dfs(int x, int y, char label, char* visited) {
    if (v[y * offset + x] == 0)
        return;
    if (x < 0 || x >= xF + 2 || y < 0 || y >= yF + 2 || visited[y * offset + x] == 1) {
        return;
    }
    visited[y * offset + x] = 1;
    v[y * offset + x] = label;
    for (int i = 0; i < 4; ++i) {
        int newX = x + dx[i];
        int newY = y + dy[i];
        dfs(newX, newY, label, visited);
    }
}

// search islands
void Bitmap::fresh() {
    char* visited = (char*)malloc(cells * sizeof(char));
    if (visited == nullptr) {
        cout << "wrongly allocate." << endl;
        exit(999);
    }
//    cout << "debug in fresh" << endl;
    memset(visited, (char)0, cells * sizeof(char));
    char label = 1;
    for (int i = 0; i < xF + 2; ++i) {
        for (int j = 0; j < yF + 2; ++j) {
            if (visited[j * offset + i] == 0 && v[j * offset + i] != 0) {
                dfs(i, j, label, visited);
                ++label;
            }
        }
    }
    free(visited);
//    cout << "debug out freshh" << endl;
}

pair<long, long> Bitmap::get_corner_but(long x, long y) {
    // not overflow because m = n = 1 and short circuit mechanism
    for (int i = m; i < m + xF; i++){
        for (int j = y + n; j < n + yF; j++){
            // not the same
            if (i == x + m && j == y + n)
                continue;
            // corner
            if(v[j * offset + i] != 0 && v[j * offset + i + 1] != 0 && v[(j + 1) * offset + i] != 0 && v[(j + 1) * offset + i +  1] != 0) { // II is free
                if (v[j * offset + i - 1] != 0 && v[(j-1) * offset + i] != 0 && v[(j-1) * offset + i - 1] == 0) { // IV is full, rt
                    return pair<long, long>{i - m, j - n};
                } else if (v[j * offset + i + 1] != 0 && v[(j-1) * offset + i] != 0 && v[(j-1) * offset + i + 1] == 0) {// III is full, lt
                    return pair<long, long>{i - m, j - n};
                } else if (v[j * offset + i - 1] != 0 && v[(j+1) * offset + i] != 0 && v[(j+1) * offset + i + 1] == 0) { // I if full, rb
                    return pair<long, long>{i - m, j - n};
                }
            }
        }
    }
    return pair<long, long>{-1, -1};
}

// transparent functions

long Bitmap::size_x() {
    return xF;
}

long Bitmap::size_y() {
    return yF;
}

char Bitmap::real_at(long di, long dj) {
    return v[(dj + n) * offset + di + m];
}

shared_ptr<string> Bitmap::to_real_string() {
    shared_ptr<string> re = std::make_shared<string>();
    for (int j = yF + n - 1; j >= n; j--) {
        re->append("row" + std::to_string(j - n) + ":\t");
        for (int i = m; i < xF + m; i++) {
            re->append(std::to_string(v[offset * j + i])+"\t");
        }
        re->push_back('\n');
    }
    re->append("     \t");
    for (int i = 0; i < xF; i++){
        re->append(std::to_string(i)+"\t");
    }
    re->push_back('\n');
    return re;
}