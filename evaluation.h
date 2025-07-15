#pragma once
#include "module.h"

namespace Evaluation{
    void init();
    void result(Ctx& ctx);
    void accept();
    void reject();
    double acceptance();
    double utilization(Ctx& ctx);
    double wastes(Ctx& ctx);
    void time_trend(Ctx& ctx);
    int acc();
    int all();
    void ACC(long, long);
    void AC();
}