#include "CubePack.h"
#include <memory>

int CubePack::apply(Ctx &ctx, Sol &sol, Task& t){
    if (t.ot == 6 && t.e == 8)
        cout << endl;
    DiagramIter start, end;
    if (sol[0] != -1) {
        t.t = t.best_t;
        start = ctx.diagrams.lower_bound(t.best_t);
        if (start->first != t.best_t) { // not found
            start--;
            ctx.diagrams[t.best_t] = std::make_shared<Diagram>(*start->second);
            start++;
        }
        end = ctx.diagrams.lower_bound(t.best_t + t.e);
        if (end->first != t.best_t + t.e || end == ctx.diagrams.end()) { // not fount or found the last
            end--;
            ctx.diagrams[t.best_t + t.e] = std::make_shared<Diagram>(*end->second);
            end++;
        }
    }
    shared_ptr<Bitmap> s = std::make_shared<Bitmap>();
    s->alloc(sol[0], sol[1], sol[2], sol[3]);
    for (auto i = start; i != end; i++) {
        i->second->apply(s, sol[2] * sol[3] - t.area);
    }
    return 0;
}

// int multi_apply(Ctx &ctx, DiagramIter& start, DiagramIter& end, Sol &sol, double ui) {
//     Bitmap *s = new Bitmap();
//     s->alloc(sol[0], sol[1], sol[2], sol[3]);
//     for (DiagramIter i = start; i != end; i++) {
//         i->second->v->pile_up(s);
//         i->second->collect_distribute(ui);
//     }
//     delete s;
//     return 0;
// }

// int cancel_apply(Ctx &ctx, DiagramIter start, DiagramIter end, Sol &sol) {
//     Bitmap *s = new Bitmap();
//     for (DiagramIter i = start; i!= end; i++) {
//         i->second->v->reverse(sol[0], sol[1], sol[2], sol[3]);
//     }
//     delete s;
//     return 0;
// }

// int multi_cancel_apply(Ctx &ctx, DiagramIter& start, DiagramIter& end, Sol& sol, double oi) {
//     Bitmap *s = new Bitmap();
//     for (DiagramIter i = start; i!= end; i++) {
//         i->second->v->reverse(sol[0], sol[1], sol[2], sol[3]);
//         i->second->leaving(oi);
//     }
//     delete s;
//     return 0;
// }

// this is an origin relaxation version, but time consuming
// only process relaxation. failure retry will process in main
// Multi-thread. Notice thread security
void alloc2(Ctx& ctx, shared_ptr<Task> t, Sol &best_sol, long relaxation) {
    t->credit = 2.0;
    Sol sol(-1,-1,-1,-1);
    while (relaxation >= 0 && t->t <= t->ddl){ // do relaxation
        bool insert_m = false, insert_n = false;
        auto tm= ctx.diagrams.lower_bound(t->t);
        if (tm->first != t->t) { // not found
            tm--;
            ctx.diagrams[t->t] = std::make_shared<Diagram>(*tm->second);
            tm++;
            insert_m = true;
        }
        auto tn = ctx.diagrams.lower_bound(t->t + t->e);
        if (tn->first != t->t + t->e || tn == ctx.diagrams.end()) { // not found
            tn--;
            ctx.diagrams[t->t + t->e] = std::make_shared<Diagram>(*tn->second);
            tn++;
            insert_n = true;
        }
        shared_ptr<Bitmap> sc = std::make_shared<Bitmap>();
        auto c = tm;
        while (c != tn) {
            sc->pile_up_no_fresh(c->second->states);
            c++;
        }
        sc->fresh();
        sc->solid_boundary();
        ctx.seq.first = tm;
        ctx.seq.second = tn;
        // send s_c, ctx, tasks to optimizers, get sol
        double candidate_bc = optimize(ctx, sol, sc, *t);
        long cur_t = t->t;
        // relaxation or failure
        // go back
        if (insert_m)
            ctx.diagrams.erase(tm->first);
        if (insert_n)
            ctx.diagrams.erase(tn->first);
        // update t->t
        tm= ctx.diagrams.lower_bound(t->t);
        if (tm != ctx.diagrams.end())
            tm++;
        if (tm == ctx.diagrams.end()) {
            relaxation -= t->ddl - t->ot;
            t->t += t->ddl - t->ot;
        } else {
            relaxation -= tm->first - t->t;
            t->t = tm->first;
        }

        // relaxation loop if success
        if (sol[0] != -1) {
            if (candidate_bc < t->credit) {
                t->best_t = cur_t;
                t->credit = candidate_bc;
                memcpy(&best_sol, &sol, sizeof(Sol));
            }
        } else { // failure loop if un-success
            break;
        }

        if (t->ddl == t->ot)
            break;
    }
}


// Multi-thread. Notice thread security. 1 task <-> 1 alloc2_in
void CubePack::alloc2_in(Ctx &ctx, shared_ptr<Task> t, Sol &sol, long relaxation){
    while (t->t <= t->ddl) { // do ddl
        // ctx is common for all threads
        Ctx new_ctx(ctx);
        auto start = std::chrono::high_resolution_clock::now();
        alloc2(new_ctx, t, sol, relaxation);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        ctx.add_time(duration, t->label);
        if (sol[0] != -1) // success in relaxation
            break;
        WARNING("Task ", *t->to_string(), "delayed negatively");
        if (t->ddl == t->ot)
            break;
//        cout << "DEBUG,ALLOC2_IN" << endl;
    }
//    cout << "debug, out of while in alloc2_in" << endl;
}

// brief relaxation version
double CubePack::alloc(Ctx &ctx, TaskPtr t, Sol &sol, long relaxation) {
    // get s_c, [t_m, t_n], t_m<=t, t_m+1>t; t_n<t+e, t_n+1>=t+e
    bool insert_m = false, insert_n = false;
    shared_ptr<Bitmap> s_c = std::make_shared<Bitmap>();
    DiagramIter t_m, t_n;
    long start = t->t, end = t->t + t->e;
    while (relaxation >= 0 && start <= t->ddl){
        auto candidate_tm= ctx.diagrams.lower_bound(start);
        auto candidate_tn = ctx.diagrams.lower_bound(end);
        shared_ptr<Bitmap> candidate_sc = std::make_shared<Bitmap>();
        if (candidate_tm->first != start) { // not found
            candidate_tm--;
            ctx.diagrams[start] = std::make_shared<Diagram>(*candidate_tm->second);
            candidate_tm++;
            insert_m = true;
        }
        if (candidate_tn->first != end) { // not found
            candidate_tn--;
            ctx.diagrams[end] = std::make_shared<Diagram>(*candidate_tn->second);
            candidate_tn++;
            insert_n = true;
        }
        auto c = candidate_tm;
        while (c != candidate_tn) {
            candidate_sc->pile_up_no_fresh(c->second->states);
            c++;
        }
        // TODO: quality designing
        if (candidate_sc->quality() > s_c ->quality()) {
            s_c = candidate_sc->shallow_copy();
            t_m = candidate_tm;
            t_n = candidate_tn;
        }

        auto next = candidate_tm;
        next++;
        relaxation -= next->first - candidate_tm->first;
        start += next->first;
        end += next->first;
    }
    s_c->fresh();
    s_c->solid_boundary();
    // send s_c, ctx, tasks to optimizers, get sol
    double best_cost = optimize(ctx, sol, s_c, *t);
    // change state in ctx
    if (sol[0] != -1) {
        apply(ctx, sol, *t);
    } else { // fail, try follow until dll exhausted
        // failure
        auto next = t_m;
        next++;
        relaxation -= next->first - t_m->first;
        t->t += next->first;
        // go back
        if (insert_m)
            ctx.diagrams.erase(t_m);
        if (insert_n)
            ctx.diagrams.erase(t_n);
    }
    return best_cost;
}

void CubePack::alloc_part(Ctx &ctx, TaskPtr& t, Sol &sol, DiagramIter& _t_m, DiagramIter& _t_n) {
    // get s_c, [t_m, t_n], t_m<=t, t_m+1>t; t_n<t+e, t_n+1>=t+e
    auto t_m = ctx.diagrams.lower_bound(t->t);
    auto t_n = ctx.diagrams.lower_bound(t->t + t->e);
    if (t_m != ctx.diagrams.begin())
        t_m--;
    if (t_n != ctx.diagrams.begin())
        t_n--;
    if (t_m->first != t->t) {
        ctx.diagrams[t->t] = std::make_shared<Diagram>(*t_m->second);
        t_m++;
    }
    if (t_n->first != t->t + t->e) {
        ctx.diagrams[t->t + t->e] = std::make_shared<Diagram>(*t_n->second);
        t_n++;
    }
    auto c = t_m;
    shared_ptr<Bitmap> s_c = std::make_shared<Bitmap>();
    while (c != t_n) {
        s_c->pile_up(c->second->states);
        c++;
    }
    // send s_c, ctx, tasks to optimizers, get sol
    optimize(ctx, sol, s_c, *t);
    if (sol[0] != -1) {
        _t_m = t_m;
        _t_n = t_n;
        t_m = ctx.diagrams.find(t->t);
        ctx.diagrams.erase(t_m);
        t_n = ctx.diagrams.find(t->t + t->e);
        ctx.diagrams.erase(t_n);
    } else {
        t_m++;
        int next = t_m->first;
        if (t->ddl >= next - t->t) {
            t->ddl = next - t->t;
            alloc_part(ctx, t, sol, _t_m, _t_n);
        }
        t_m = ctx.diagrams.find(t->t);
        ctx.diagrams.erase(t_m);
        t_n = ctx.diagrams.find(t->t + t->e);
        ctx.diagrams.erase(t_n);
    }
}

void CubePack::alloc_para(Ctx &ctx, TaskPool& tasks, vector<Sol> &sols_re, long relaxation) {
    ThreadPool thp = ThreadPool(tasks.size());
    vector<DiagramIter> t_ms(tasks.size());
    vector<DiagramIter> t_ns(tasks.size());
    vector<Sol> sols(tasks.size(), {-1,-1,-1,-1});
    vector<int> ddls(tasks.size());
    for (auto& task: tasks)
        ddls.push_back(task->ddl);

    for (int i = 0; i < tasks.size(); i++)
        thp.enqueue(CubePack::alloc_part, ref(ctx), ref(tasks[i]), ref(sols[i]), ref(t_ms[i]), ref(t_ns[i]));
    thp.wait();

    // each task in tasks compute its own best solution based on a relaxation and ddl

    bool f = true;
    while (f) {
        f = false;
        int lowest = -1;
        double lowest_cost = 2.0;
        for (int i = 0; i < tasks.size(); i++) {
            if (tasks[i]->credit < lowest_cost) {
                lowest_cost = tasks[i]->credit;
                lowest = i;
            }
            f = f || lowest != -1;
        }


        if (lowest != -1) {
            CubePack::apply(ctx, sols[lowest], *tasks[lowest]);
            Evaluation::accept();
            memcpy(&sols_re[lowest], &sols[lowest], sizeof(Sol));
            INFO("Accept:" + *sols_re[lowest].to_string());
            tasks.erase(tasks.begin() + lowest);
            t_ms.erase(t_ms.begin() + lowest);
            t_ns.erase(t_ns.begin() + lowest);
            ddls.erase(ddls.begin() + lowest);
            sols.erase(sols.begin() + lowest);
        } else {
            Evaluation::reject();
            WARNING("Reject:" + *tasks[lowest]->to_string());
        }
        for (int i = 0; i < ddls.size(); i++)
            tasks[i]->ddl = ddls[i];
    }
}


// int multi_alloc(Ctx &ctx, vector<Task> &tasks, vector<Sol> &sols) {
//     vector<double> ui(tasks.size(), 0.0);
//     vector<double> oi(tasks.size(), 0.0);
//     // search NEs
//     for (int i = 0; i < atoi(ctx.param["search_iter"].c_str()); i++) {
//         //init a policy set
//         for (int t = 0; t < tasks.size(); t++){
//             // get s_c, [t_m, t_n], t_m<=t, t_m+1>t; t_n<t+e, t_n+1>=t+e
//             DiagramIter t_m = ctx.s->lower_bound(tasks[t].t);
//             DiagramIter t_n = ctx.s->lower_bound(tasks[t].t + tasks[t].e);
//             if (t_m->first != tasks[t].t) {
//                 ctx.s->at(tasks[t].t) = t_m->second->copy();
//                 t_m++;
//             }
//             if (t_n->first != tasks[t].t + tasks[t].e) {
//                 ctx.s->at(tasks[t].t + tasks[t].e) = t_n->second->copy();
//                 t_n++;
//             }
//             DiagramIter c = t_m;
//             Bitmap* s_c = new Bitmap();
//             while (c != t_n) {
//                 s_c->pile_up(c->second->v);
//                 c++;
//             }
//             game_construct(s_c, t_m, t_n);
//             // send s_c, ctx, tasks to optimizers, get sol
//             ui[i] = multi_optimize(ctx, tasks[t], sols[t]);
//             delete s_c;
//             // composite states
//             multi_apply(ctx, t_m, t_n, sols[t], ui[i]);
//             oi[i] = ui[i];
//         }
//         //iteration search
//         for (int t = 0; t < tasks.size(); t++) {
//             // get s_c, [t_m, t_n], t_m<=t, t_m+1>t; t_n<t+e, t_n+1>=t+e
//             auto t_m = ctx.s->lower_bound(tasks[t].t);
//             auto t_n = ctx.s->lower_bound(tasks[t].t + tasks[t].e);
//             // decompiste states
//             multi_cancel_apply(ctx, t_m, t_n, sols[t], oi[i]);
//             auto c = t_m;
//             Bitmap* s_c = new Bitmap();
//             while (c != t_n) {
//                 s_c->pile_up(c->second->v);
//                 c++;
//             }
//             game_construct(s_c, t_m, t_n);
//             // send s_c, ctx, tasks to optimizers, get sol
//             ui[i] = multi_optimize(ctx, tasks[t], sols[t]);
//             delete s_c;
//             // composite states
//             multi_apply(ctx, t_m, t_n, sols[t], ui[i]);
//             oi[i] = ui[i];
//         }
//     }
//     return 0;
// }