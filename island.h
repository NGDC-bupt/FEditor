#pragma once

#include <memory>
#include <vector>
#include <map>
using namespace std;

enum Direction {
    Up,
    Down,
    Left,
    Right
};

class VertexNode {
public:
    long x, y;
    Direction dir;
    shared_ptr<VertexNode> next;

    VertexNode(int x, int y, Direction dir)
            : x(x), y(y), dir(dir), next(nullptr) {}

    void updateDirection();
};


namespace VertexOP {
    bool areVectorsOverlapping(const shared_ptr<VertexNode>& A, const shared_ptr<VertexNode>& B,
                               const shared_ptr<VertexNode>& C, const shared_ptr<VertexNode>& D);
    // shared_ptr<VertexNode> findMinXYVertex(shared_ptr<VertexNode> v1, shared_ptr<VertexNode> v2,
    //                               shared_ptr<VertexNode> v3, shared_ptr<VertexNode> v4);
    // shared_ptr<VertexNode> VertexOP::findMaxXYVertex(shared_ptr<VertexNode> v1, shared_ptr<VertexNode> v2,
    //                                         shared_ptr<VertexNode> v3, shared_ptr<VertexNode> v4);
    void adjustPointersForOverlap(shared_ptr<VertexNode> &v1, shared_ptr<VertexNode> &v2);
}

enum RANGE
{
    min_x,
    max_x,
    min_y,
    max_y
};

class Sol {
public:
    long value[4];
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
};

class Island {
public:
    shared_ptr<VertexNode> head;
    shared_ptr<VertexNode> tail;
    long range[4];// min_x max_x min_y max_y

    Island() {};
    Island(pair<long,long> boundary) {
        int x = boundary.first, y = boundary.second;
        range[RANGE::min_x] = 0;
        range[RANGE::max_x] = x;
        range[RANGE::min_y] = 0;
        range[RANGE::max_y] = y;
        head = shared_ptr<VertexNode>(new VertexNode(0,0,Direction::Right));
        head->next = shared_ptr<VertexNode>(new VertexNode(x,0,Direction::Up));
        head->next->next = shared_ptr<VertexNode>(new VertexNode(x,y,Direction::Left));
        head->next->next->next = shared_ptr<VertexNode>(new VertexNode(0,y,Direction::Down));
        head->next->next->next->next = head;
        tail = head->next->next->next;
    };
    Island(shared_ptr<Island>& island) {
        memcpy(range, island->range, sizeof(range));
        head = shared_ptr<VertexNode>(new VertexNode(island->head->x, island->head->y, island->head->dir));
        shared_ptr<VertexNode> prev = head;
        shared_ptr<VertexNode> curr = island->head->next;
        while (curr != island->head) {
            shared_ptr<VertexNode> node = shared_ptr<VertexNode>(new VertexNode(curr->x, curr->y, curr->dir));
            prev->next = node;
            prev = node;
            curr = curr->next;
        }
        prev->next = head;
        tail = prev;
    };
    bool outside(shared_ptr<Island>& s);
    bool inside(shared_ptr<Island>& s);
    shared_ptr<string> to_string();
    shared_ptr<vector<shared_ptr<VertexNode>>> to_vector();

    long get_min_rec() {
        return (range[RANGE::max_x] - range[RANGE::min_x]) * (range[RANGE::max_y] - range[RANGE::min_y]);
    }
};

namespace Solution {
    shared_ptr<Island> createIslandForSolution(Sol& sol);
    shared_ptr<Island> createIslandForSolutionTran(Sol &sol);
}

namespace IslandOP {
    vector<shared_ptr<Island>> findIslands(shared_ptr<VertexNode> island);
}

//typedef vector<shared_ptr<Island>> Islands;

namespace IslandsOP {
    vector<shared_ptr<Island>> copy(vector<shared_ptr<Island>>& is);
    void insertIslands(vector<shared_ptr<Island>>& islands, vector<shared_ptr<VertexNode>>& nodes);
    void updateForInsertion(vector<shared_ptr<Island>>& islands, Sol &sol);
    void updateForFree(vector<shared_ptr<Island>>& islands, Sol &sol);
};