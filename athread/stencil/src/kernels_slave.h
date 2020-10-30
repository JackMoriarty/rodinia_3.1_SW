#ifndef _KERNEL_SLAVE_H_
#define _KERNEL_SLAVE_H_

struct spe_param {
    float c0;
    float c1;
    float *A0;
    float *Anext;
    int nx;
    int ny;
    int nz;
};

#endif /* _KERNEL_SLAVE_H_ */
