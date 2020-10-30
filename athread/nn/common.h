#ifndef _COMMON_H_
#define _COMMON_H_

struct spe_parameter {
    int rec_count;
    char *sandbox;
    float *z;
    float target_lat;
    float target_long;
};

extern void slave_nn_slave(struct spe_parameter *spe_param);

#endif