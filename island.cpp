#include "island.h"
#include <cmath>
#include <unordered_set>

map<int, string> DirectionMap = {
        {Direction::Up, "Up"},
        {Direction::Down, "Down"},
        {Direction::Left, "Left"},
        {Direction::Right, "Right"}};

/* ---------------- VertexNode begin ---------------- */

void VertexNode::updateDirection() {
    // eliminate this != nullptr
    if (!this->next) return;
    // self lock unchange direction
    if (this->next.get() == this)
        return;

    int dx = this->next->x - this->x;
    int dy = this->next->y - this->y;

    if (dx != 0)
        this->dir = (dx > 0) ? Direction::Right : Direction::Left;
    else if (dy != 0)
        this->dir = (dy > 0) ? Direction::Up : Direction::Down;
}

/* ---------------- VertexNode end ---------------- */
/* ---------------- VertexOP begin ---------------- */

bool VertexOP::areVectorsOverlapping(const shared_ptr<VertexNode>& A, const shared_ptr<VertexNode>& B,
                                     const shared_ptr<VertexNode>& C, const shared_ptr<VertexNode>& D) {
    if (A->dir == Direction::Left || A->dir == Direction::Right)
        return max(A->x, B->x) >= min(C->x, D->x) && max(C->x, D->x) >= min(A->x, B->x);
    else if (A->dir == Direction::Up || A->dir == Direction::Down)
        return max(A->y, B->y) >= min(C->y, D->y) && max(C->y, D->y) >= min(A->y, B->y);
    return false;
}

// shared_ptr<VertexNode> VertexOP::findMinXYVertex(shared_ptr<VertexNode> v1, shared_ptr<VertexNode> v2,
//     shared_ptr<VertexNode> v3, shared_ptr<VertexNode> v4) {
//     shared_ptr<VertexNode> minVertex = v1;
//     if (v1->dir == Direction::Up || v1->dir == Direction::Down) {
//         if (v2->yF < minVertex->yF) minVertex = v2;
//         if (v3->yF < minVertex->yF) minVertex = v3;
//         if (v4->yF < minVertex->yF) minVertex = v4;

//     }
//     else if(v1->dir == Direction::Left || v1->dir == Direction::Right) {
//         if (v2->xF < minVertex->xF) minVertex = v2;
//         if (v3->xF < minVertex->xF) minVertex = v3;
//         if (v4->xF < minVertex->xF) minVertex = v4;
//     }
//     return minVertex;
// }

// shared_ptr<VertexNode> VertexOP::findMaxXYVertex(shared_ptr<VertexNode> v1, shared_ptr<VertexNode> v2,
//                                            shared_ptr<VertexNode> v3, shared_ptr<VertexNode> v4) {
//     shared_ptr<VertexNode> maxVertex = v1;
//     if (v1->dir == Direction::Up || v1->dir == Direction::Down) {
//         if (v2->yF > maxVertex->yF) maxVertex = v2;
//         if (v3->yF > maxVertex->yF) maxVertex = v3;
//         if (v4->yF > maxVertex->yF) maxVertex = v4;

//     }
//     else if(v1->dir == Direction::Left || v1->dir == Direction::Right){
//         if (v2->xF > maxVertex->xF) maxVertex = v2;
//         if (v3->xF > maxVertex->xF) maxVertex = v3;
//         if (v4->xF > maxVertex->xF) maxVertex = v4;
//     }
//     return maxVertex;
// }

// v2 is always a vertex of rectangle
void VertexOP::adjustPointersForOverlap(shared_ptr<VertexNode>& v1, shared_ptr<VertexNode>& v2) {
    if (v1->dir != v2->dir) { // reverse direction
        shared_ptr<VertexNode> v = v1->next;
        v1->next = v2->next;
        v2->next = v;
        v1->updateDirection();
        v2->updateDirection();
    } else { // same direction
        if (v1->dir == Direction::Up) {
            v2->y = v1->y < v2->y ? v1->y : v2->y;
            v2->next->y = v1->next->y > v2->next->y ? v1->next->y : v2->next->y;
            v1->next = v1;
        } else if (v1->dir == Direction::Down) {
            v2->y = v1->y > v2->y ? v1->y : v2->y;
            v2->next->y = v1->next->y < v2->next->y ? v1->next->y : v2->next->y;
            v1->next = v1;
        } else if (v1->dir == Direction::Left) {
            v2->x = v1->x > v2->x ? v1->x : v2->x;
            v2->next->x = v1->next->x < v2->next->x ? v1->next->x : v2->next->x;
            v1->next = v1;
        } else {// Direction::Right
            v2->x = v1->x < v2->x ? v1->x : v2->x;
            v2->next->x = v1->next->x > v2->next->x ? v1->next->x : v2->next->x;
            v1->next = v1;
        }
    }
}

/* ---------------- VertexOP end ---------------- */
/* ---------------- Solution begin ---------------- */

// shared_ptr<VertexNode> Solution::findVertexByXAndDirection(int targetX, Direction targetDirection, Direction targetDirection2) {
//     shared_ptr<VertexNode> current = head;
//     while (current != nullptr) {
//         if (current->xF == targetX &&
//             (current->dir == targetDirection || current->dir == targetDirection2)) {
//             return current;
//         }
//         current = current->next;
//     }
//     return nullptr;
// }

// shared_ptr<VertexNode> Solution::findVertexByYAndDirection(int targetY, Direction targetDirection, Direction targetDirection2) {
//     shared_ptr<VertexNode> current = head;
//     while (current != nullptr) {
//         if (current->yF == targetY &&
//             (current->dir == targetDirection || current->dir == targetDirection2)) {
//             return current;
//         }
//         current = current->next;
//     }
//     return nullptr;
// }

shared_ptr<Island> Solution::createIslandForSolution(Sol& sol) {
    shared_ptr<VertexNode> lb = shared_ptr<VertexNode>(new VertexNode(sol[0], sol[1], Direction::Up));
    shared_ptr<VertexNode> lt = shared_ptr<VertexNode>(new VertexNode(sol[0], sol[1] + sol[3], Direction::Right));
    shared_ptr<VertexNode> rt = shared_ptr<VertexNode>(new VertexNode(sol[0] + sol[2], sol[1] + sol[3], Direction::Down));
    shared_ptr<VertexNode> rb = shared_ptr<VertexNode>(new VertexNode(sol[0] + sol[2], sol[1], Direction::Left));
    lb->next = lt;
    lt->next = rt;
    rt->next = rb;
    rb->next = lb;
    shared_ptr<Island> island = shared_ptr<Island>(new Island());
    island->head = lb;
    island->tail = rb;
    island->range[RANGE::min_x] = sol[0];
    island->range[RANGE::min_y] = sol[1];
    island->range[RANGE::max_x] = sol[0] + sol[2];
    island->range[RANGE::max_y] = sol[1] + sol[3];
    return island;
}

shared_ptr<Island> Solution::createIslandForSolutionTran(Sol& sol) {
    shared_ptr<VertexNode> lb = shared_ptr<VertexNode>(new VertexNode(sol[0], sol[1], Direction::Right));
    shared_ptr<VertexNode> lt = shared_ptr<VertexNode>(new VertexNode(sol[0], sol[1] + sol[3], Direction::Down));
    shared_ptr<VertexNode> rt = shared_ptr<VertexNode>(new VertexNode(sol[0] + sol[2], sol[1] + sol[3], Direction::Left));
    shared_ptr<VertexNode> rb = shared_ptr<VertexNode>(new VertexNode(sol[0] + sol[2], sol[1], Direction::Up));
    lb->next = rb;
    rb->next = rt;
    rt->next = lt;
    lt->next = lb;
    shared_ptr<Island> island = shared_ptr<Island>(new Island());
    island->head = lb;
    island->tail = lt;
    island->range[RANGE::min_x] = sol[0];
    island->range[RANGE::min_y] = sol[1];
    island->range[RANGE::max_x] = sol[0] + sol[2];
    island->range[RANGE::max_y] = sol[1] + sol[3];
    return island;
}

/* ---------------- Solution end ---------------- */
/* ---------------- Island begin ---------------- */

bool Island::outside(shared_ptr<Island>& sol) {
    return (sol->range[RANGE::max_x] < range[RANGE::min_x] || sol->range[RANGE::min_x] > range[RANGE::min_x] 
            || sol->range[RANGE::max_y] < range[RANGE::max_y] || sol->range[RANGE::min_y] > range[RANGE::min_y]);
}

bool Island::inside(shared_ptr<Island>& sol) {
    return (sol->range[RANGE::max_x] <= range[RANGE::max_x] && sol->range[RANGE::min_x] >= range[RANGE::min_x] 
            && sol->range[RANGE::max_y] <= range[RANGE::max_y] && sol->range[RANGE::min_y] >= range[RANGE::min_y]);
}

shared_ptr<string> Island::to_string() {
    shared_ptr<string> re = shared_ptr<string>(new string());
    shared_ptr<VertexNode> cur = head;
    bool loop = false;
    while (!loop) {
        loop = cur->next == head;
        re->append("(" + std::to_string(cur->x) + "," + std::to_string(cur->y) + "," + DirectionMap[cur->dir] + ")");
        if (!loop)
            re->append(" -> ");
        cur = cur->next;
    }
    re->append("\n");
    return re;
}

shared_ptr<vector<shared_ptr<VertexNode>>> Island::to_vector() {
    shared_ptr<vector<shared_ptr<VertexNode>>> re =  shared_ptr<vector<shared_ptr<VertexNode>>>(new vector<shared_ptr<VertexNode>>());
    shared_ptr<VertexNode> cur = head;
    bool loop = false;
    while (!loop) {
        loop = cur->next == head;
        re->push_back(cur);
        cur = cur->next;
    }
    return re;
}

/* ---------------- Island end ---------------- */
/* ---------------- Islands begin ---------------- */

vector<shared_ptr<Island>> IslandsOP::copy(vector<shared_ptr<Island>>& is) {
    vector<shared_ptr<Island>> re;
    for (auto& i: is)
        re.push_back(shared_ptr<Island>(new Island(i)));
    return re;
}

void IslandsOP::insertIslands(vector<shared_ptr<Island>>& islands, vector<shared_ptr<VertexNode>>& nodes) {
    unordered_set<VertexNode*> v;
    for (auto& n: nodes) {
        if (v.count(n.get()) != 0)
            continue;
        v.insert(n.get());
        shared_ptr<Island> island = shared_ptr<Island>(new Island());
        island->head = n;
        shared_ptr<VertexNode> cur = n;
        while (v.count(cur.get()) == 0) {
            island->tail = cur;
            v.insert(cur.get());
            island->range[RANGE::min_x] = min(cur->x, island->range[RANGE::min_x]);
            island->range[RANGE::min_y] = min(cur->y, island->range[RANGE::min_y]);
            island->range[RANGE::max_x] = min(cur->x, island->range[RANGE::max_x]);
            island->range[RANGE::max_y] = min(cur->y, island->range[RANGE::max_y]);
            cur = cur->next;
        }
        islands.push_back(island);
    }
}

inline bool isSelfSpin(shared_ptr<VertexNode> v) {
    return v->next == v;
}

inline void swap(shared_ptr<VertexNode>& a, shared_ptr<VertexNode>& b, shared_ptr<VertexNode>& p) {
    shared_ptr<VertexNode> t = a;
    a = b;
    b = t;
    p->next = a;
}

void IslandsOP::updateForInsertion(vector<shared_ptr<Island>>& islands, Sol& sol) {
    shared_ptr<Island> s = Solution::createIslandForSolution(sol);
    shared_ptr<VertexNode> lb = s->head;
    shared_ptr<VertexNode> lt = lb->next;
    shared_ptr<VertexNode> rt = lt->next;
    shared_ptr<VertexNode> rb = rt->next;
    vector<shared_ptr<VertexNode>> nodes;
    int i = 0;
    for (; i < islands.size(); i++)
    {
        shared_ptr<Island> island = islands[i];
        if (!island->inside(s))
            continue;
        shared_ptr<VertexNode> cur = island->head;
        shared_ptr<VertexNode> prev = island->tail;
        bool loop_twice = false;
        while (!loop_twice) {
            loop_twice = cur->next == island->head;
            shared_ptr next = cur->next;
            if (cur->dir == Direction::Up || cur->dir == Direction::Down) {
                if (VertexOP::areVectorsOverlapping(cur, cur->next, lb, lt)) {
                    VertexOP::adjustPointersForOverlap(cur, lb);
                    if (isSelfSpin(lb) && !isSelfSpin(cur))
                        swap(lb, cur, prev);
                    if (!isSelfSpin(cur))
                        nodes.push_back(cur);
                } else if (VertexOP::areVectorsOverlapping(cur, cur->next, rt, rb)) {
                    VertexOP::adjustPointersForOverlap(cur, rt);
                    if (isSelfSpin(rt) && !isSelfSpin(cur))
                        swap(rt, cur, prev);
                    if (!isSelfSpin(cur))
                        nodes.push_back(cur);
                }
            } else if (cur->dir == Direction::Left || cur->dir == Direction::Right) {
                if (VertexOP::areVectorsOverlapping(cur, cur->next, lt, rt)) {
                    VertexOP::adjustPointersForOverlap(cur, lt);
                    if (isSelfSpin(lt) && !isSelfSpin(cur))
                        swap(lt, cur, prev);
                    if (!isSelfSpin(cur))
                        nodes.push_back(cur);
                } else if (VertexOP::areVectorsOverlapping(cur, cur->next, rb, lb)) {
                    VertexOP::adjustPointersForOverlap(cur, rb);
                    if (isSelfSpin(rb) && !isSelfSpin(cur))
                        swap(rb, cur, prev);
                    if (!isSelfSpin(cur))
                        nodes.push_back(cur);
                }
            }
            cur = next;
        }

        if (!nodes.empty())
            break;
    }
    if (!isSelfSpin(lb))
        nodes.push_back(lb);
    if (!isSelfSpin(lt))
        nodes.push_back(lt);
    if (!isSelfSpin(rt))
        nodes.push_back(rt);
    if (!isSelfSpin(rb))
        nodes.push_back(rb);
    islands.erase(islands.begin() + i);
    insertIslands(islands, nodes);
}

void IslandsOP::updateForFree(vector<shared_ptr<Island>>& islands, Sol& sol) {
    shared_ptr<Island> s = Solution::createIslandForSolutionTran(sol);
    shared_ptr<VertexNode> lb = s->head;
    shared_ptr<VertexNode> rb = lb->next;
    shared_ptr<VertexNode> rt = rb->next;
    shared_ptr<VertexNode> lt = rt->next;
    vector<shared_ptr<VertexNode>> nodes;
    vector<int> related_islands;
    shared_ptr<VertexNode> leftmost = lb;
    for (int i = 0; i < islands.size(); i++)
    {
        shared_ptr<Island> island = islands[i];
        if (!island->outside(s))
            continue;
        shared_ptr<VertexNode> cur = island->head;
        shared_ptr<VertexNode> prev = island->tail;
        bool loop_twice = false;
        bool related = false;
        while (!loop_twice) {
            loop_twice = cur->next == island->head;
            shared_ptr next = cur->next;
            if (cur->dir == Direction::Up || cur->dir == Direction::Down) {
                if (VertexOP::areVectorsOverlapping(cur, cur->next, lb, lt)) {
                    related = true;
                    VertexOP::adjustPointersForOverlap(cur, lb);
                    if (isSelfSpin(lb) && !isSelfSpin(cur))
                        swap(lb, cur, prev);
                } else if (VertexOP::areVectorsOverlapping(cur, cur->next, rt, rb)) {
                    related = true;
                    VertexOP::adjustPointersForOverlap(cur, rt);
                    if (isSelfSpin(rt) && !isSelfSpin(cur))
                        swap(rt, cur, prev);
                }
            } else if (cur->dir == Direction::Left || cur->dir == Direction::Right) {
                if (VertexOP::areVectorsOverlapping(cur, cur->next, lt, rt)) {
                    related = true;
                    VertexOP::adjustPointersForOverlap(cur, lt);
                    if (isSelfSpin(lt) && !isSelfSpin(cur))
                        swap(lt, cur, prev);
                } else if (VertexOP::areVectorsOverlapping(cur, cur->next, rb, lb)) {
                    related = true;
                    VertexOP::adjustPointersForOverlap(cur, rb);
                    if (isSelfSpin(rb) && !isSelfSpin(cur))
                        swap(rb, cur, prev);
                }
            }
            cur = next;
        }

        if (related)
            related_islands.push_back(i);
    }

    for (auto i: related_islands)
        islands.erase(islands.begin() + i);
    insertIslands(islands, nodes);
}