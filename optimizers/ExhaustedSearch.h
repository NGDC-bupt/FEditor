#pragma once
#include "optimizers.h"
#include <mutex>

// ctx, tasks, sol, s_c
void ES_init(Ctx& ctx);
double ES_entry(Ctx &ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t);
double ES_entry_p(Ctx &ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t);