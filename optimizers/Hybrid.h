#ifndef HYBRID_
#define HYBRID_
#include "optimizers.h"
#include <mutex>
#include <vector>
// split the solution space
// SA-define xF,yF   PSO-define w,h

void Hybrid_init(Ctx& ctx);
double Hybrid_entry(Ctx &ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t);
double Hybrid_entry_p(Ctx &ctx, Sol &s, shared_ptr<Bitmap> sc, Task& t);

#endif