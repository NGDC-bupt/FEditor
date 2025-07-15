#pragma once
#include "optimizers.h"
#include <mutex>

void PSO_init(Ctx& ctx);
double PSO_entry(Ctx &ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t);
double PSO_entry_p(Ctx &ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t);