#pragma once
#include "optimizers.h"
#include <mutex>

// ctx, tasks, sol, s_c
void SA_init(Ctx& ctx);
double SA_entry(Ctx &ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t);
double SA_entry_p(Ctx &ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t);