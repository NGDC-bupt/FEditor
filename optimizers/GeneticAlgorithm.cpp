#include "GeneticAlgorithm.h"
#include <thread>
#include <cstring>

static int threads = 1;
extern MetricFunc metric;

static int GROUP_SCALE, MAX_GENS;
static long MAX_X, MAX_Y, MAX_H_SIZE, MAX_W_SIZE, MIN_H_SIZE, MIN_W_SIZE;
static double CROSSOVER_RATE, MUTATION_RATE;

struct Individual {
    Sol x;
    double fitness;
    double re_fitness;
    double sum_fitness;
};



void Xover(int one, int two,vector<struct Individual>& population)
{
    int point = rand() % 4;
    for (int i = 0; i < point; i++)
    {
        double t = population[one].x[i];
        population[one].x[i] = population[two].x[i];
        population[two].x[i] = t;
    }
}

// Mate all the father
void crossover(vector<struct Individual>& population) {
    int mem;
    int one;
    int first = 0;
    for (mem = 0; mem < GROUP_SCALE; ++mem)
    {
        double p = (rand() % 100) / 100.0;
        if (p < CROSSOVER_RATE)
        {
            ++first;
            if (first % 2 == 0)
                Xover(one, mem, population);
            else
                one = mem;
        }
    }
}

// kill those bad individuls
void elitist(vector<struct Individual>& population) {
    double best;
    int best_mem;
    double worst;
    int worst_mem;

    best = population[0].fitness;
    worst = population[0].fitness;

    for (int i = 0; i < GROUP_SCALE - 1; ++i)
    {
        if (population[i + 1].fitness < population[i].fitness)
        {

            if (best <= population[i].fitness)
            {
                best = population[i].fitness;
                best_mem = i;
            }

            if (population[i + 1].fitness <= worst)
            {
                worst = population[i + 1].fitness;
                worst_mem = i + 1;
            }

        } else {
            if (population[i].fitness <= worst)
            {
                worst = population[i].fitness;
                worst_mem = i;
            }

            if (best <= population[i + 1].fitness)
            {
                best = population[i + 1].fitness;
                best_mem = i + 1;
            }
        }
    }

    // if current best is less than last generation's best, then keep the last generation's best
    if (population[GROUP_SCALE].fitness <= best)
    {
        memcpy(&population[GROUP_SCALE].x, &population[best_mem].x, sizeof(population[best_mem].x));
        population[GROUP_SCALE].fitness = population[best_mem].fitness;
    } else {
        memcpy(&population[best_mem].x, &population[GROUP_SCALE].x, sizeof(population[GROUP_SCALE].x));
        population[worst_mem].fitness = population[GROUP_SCALE].fitness;
    }
}


inline void init_individual(struct Individual& in) {
    for (int i = 0; i < 4; i++) {
        in.x[0] = rand() % (MAX_X + 1);
        in.x[1] = rand() % (MAX_Y + 1);
        in.x[2] = rand() % (MAX_W_SIZE - MIN_W_SIZE + 1) + MIN_W_SIZE;
        in.x[3] = rand() % (MAX_H_SIZE - MIN_H_SIZE + 1) + MIN_H_SIZE;

        // Ensure the rectangular region does not exceed the FPGA boundary
        if (in.x[0] + in.x[2] > MAX_X)
            in.x[2] = MAX_X - in.x[0];
        if (in.x[1] +in.x[3] > MAX_Y)
            in.x[3] = MAX_Y - in.x[1];
    }
}

void initGroup(Ctx& ctx, vector<struct Individual>& population, int s, int e) {
    // [s,e)
    for (int j = s; j < e && j < population.size(); j++) {
        population[j].fitness = 0;
        population[j].re_fitness = 0;
        population[j].sum_fitness = 0;
        // population[j].xF[i] = rand();
        init_individual(population[j]);
        while (is_legal(ctx, population[j].x) != 0)
            init_individual(population[j]);

        population[j].fitness = metric(ctx, population[j].x);
    }
}

void selectBest(vector<struct Individual>& population) {
    int cur_best = 0;

    for (int mem = 0; mem < GROUP_SCALE; mem++)
    {
        if (population[GROUP_SCALE].fitness < population[mem].fitness)
        {
            cur_best = mem;
            population[GROUP_SCALE].fitness = population[mem].fitness;
        }
    }

    memcpy(&population[GROUP_SCALE].x, &population[cur_best].x, sizeof(population[cur_best].x));
}

void mutate(vector<struct Individual>& population) {
    for (int i = 0; i < GROUP_SCALE; i++) {
        double p = (rand() % 100) / 100.0;
        if (p < MUTATION_RATE)
            population[i].x[0] = rand() % (MAX_X + 1);

        p = (rand() % 150) / 150.0;
        if (p < MUTATION_RATE)
            population[i].x[1] = rand() % (MAX_Y + 1);

        p = (rand() % 50) / 50.0;
        if (p < MUTATION_RATE)
            population[i].x[2] = rand() % (MAX_W_SIZE - MIN_W_SIZE + 1) + MIN_W_SIZE;

        p = (rand() % 200) / 200.0;
        if (p < MUTATION_RATE)
            population[i].x[3] = rand() % (MAX_H_SIZE - MIN_H_SIZE + 1) + MIN_H_SIZE;

        // Ensure the rectangular region does not exceed the FPGA boundary
        if (population[i].x[0] + population[i].x[2] > MAX_X)
            population[i].x[2] = MAX_X - population[i].x[0];
        if (population[i].x[1] +population[i].x[3] > MAX_Y)
            population[i].x[3] = MAX_Y - population[i].x[1];
    }
}

// select who can mate
void selector(vector<struct Individual>& population) {
    vector<struct Individual> new_population(GROUP_SCALE + 1);

    double sum = 0.0;
    for (int mem = 0; mem < GROUP_SCALE; mem++)
        sum += population[mem].fitness;

    for (int mem = 0; mem < GROUP_SCALE; mem++)
        population[mem].re_fitness = population[mem].fitness / sum;

    // Round-Robin
    population[0].sum_fitness = population[0].re_fitness;
    for (int mem = 1; mem < GROUP_SCALE; mem++)
        population[mem].sum_fitness = population[mem - 1].sum_fitness + population[mem].re_fitness;

    for (int i = 0; i < GROUP_SCALE; i++)
    {
        double p = (rand() % 100) / 100.0;
        if (p < population[0].sum_fitness)
            new_population[i] = population[0];
        else
            for (int j = 0; j < GROUP_SCALE; j++)
                if (population[j].sum_fitness <= p && p < population[j + 1].sum_fitness)
                    new_population[i] = population[j + 1];
    }

    population = new_population;
}

double GA_entry(Ctx &ctx, Sol &s){
    vector<struct Individual> population(GROUP_SCALE + 1);
    initGroup(ctx, population, 0, population.size());
    selectBest(population);

    for (int gen = 0; gen < MAX_GENS; gen++) {
        selector(population);
        crossover(population);
        mutate(population);
        for (int i = 0; i < GROUP_SCALE; i++)
            population[i].fitness = metric(ctx, population[i].x);
        elitist(population);
    }

    memcpy(&s, &population[GROUP_SCALE].x, sizeof(Sol));
    return population[GROUP_SCALE].fitness;
}


double GA_entry_p(Ctx &ctx, Sol &s) {
    int ths = stoi(ctx.param["threads"]);
    vector<struct Individual> population(GROUP_SCALE + 1);
    vector<thread> thread_pool;

    for (int i = 0; i < ths; i++)
        thread_pool.emplace_back(initGroup, ref(ctx), ref(population), i * GROUP_SCALE / ths, (i + 1) * GROUP_SCALE / ths);

    for (auto& th : thread_pool)
        if (th.joinable())
            th.join();

    selectBest(population);

    for (int gen = 0; gen < MAX_GENS; gen++) {
        selector(population);
        crossover(population);
        mutate(population);
        thread_pool.clear();
        for (int i = 0; i < population.size(); i++)
            thread_pool.emplace_back(metric, ref(ctx), ref(population[i].x));

        for (auto& th : thread_pool)
            if (th.joinable())
                th.join();
        elitist(population);
    }
    return population[GROUP_SCALE].fitness;
}