### Notitaion
We have changed our name of this project to FEditor, replacing CubePack.

# CubePack
CubePack is an efficient and accurate online placement slover for heterogenous FPGA and multitasks.

## Main Idea
It is mainly based on the 2D discrete diagrams rather than 3D model. And it accelerates heuristice algorithms. Also for multitasks, CubePack introduce Game Theory to help. The cooresping paper is available at: .

## Config
All config files obeys "key:value" format while each parameter occupies a line.
### env.config
- flag: check whether the file is adapted
- opt: choose the optimizer type
- metric: choose the metric type within ABC, AB,AC,BC,A,B,C
- alpha, beta, gamma: hyperparameters for weight
- mode: only get- lcp, lcpp -- list-CubePack, list-CubePack-parallel(cpm resreved)
- select: how many for lcpp
- relaxation: time relaxation
- search_iter: only for multiple tasks, defining how many NEs will be searched.
- iteration: maximum iterations of the time passing
- test_suit: the task suit. 0 for random genertaion
- threads: for parallel computing, depending on your CPU
- sa_threads: only for hybrid one, outer * inner = threads will the optimal

### fpga.config
- flag: check whether the file is adapted
- xF: the width of FPGA
- yF: the height of FPGA
- seq: example (CLB,CLB,CLB,RAM,DSP,DSP)

### task.config
- flag: check whether the file is adapted
- task_speed: the speed of task generation
- task_e: the execution time of task
- clb: requirement for clb
- ram: requirement for ram
- dsp: requirement for dsp

### SA.config
- T: initial temperature
- T_min: the minimal temperature
- alpha: cooling rate
- max_iter: the max iteration of SA

### PSO.config
- PARTICLE_COUNT:particles
- MAX_ITER:max iteration
- INERTIA: inertia of each particle
- COGNITIVE: cognitive weight
- SOCIAL: social weight
- V_MAX: max velocity
- MAX_X: max positon in xF
- MAX_Y: max positon in yF
- MIN_W_SIZE: min width
- MAX_W_SIZE: max width
- MIN_H_SIZE: min height
- MAX_H_SIZE: max height