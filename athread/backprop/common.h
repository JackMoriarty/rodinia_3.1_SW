#ifndef _COMMON_H_
#define _COMMON_H_
struct spe_func1_parameter {
    int n1;
    int n2;
    float *l1;
    float *l2;
    float **conn;
};

struct spe_func2_parameter {
    int ndelta;
    int nly;
    float *delta;
    float *ly;
    float **w;
    float **oldw;
};
#endif
