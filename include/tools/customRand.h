#ifndef CUSTOMRAND_H
#define CUSTOMRAND_H

#include <random>
#include <thread>
#include <random/xoshiro.h>

inline thread_local xso::rng gen;

inline double random_double_01() {
    return (gen() >> 11) * 0x1.0p-53;
}


inline double random_real(double lower, double upper) {
    return lower + random_double_01() * (upper - lower);
}

inline int random_integer(int lower, int upper) {
    uint64_t range = (uint64_t)upper - lower + 1;
    return lower + (int)(gen() % range);
}

#endif // CUSTOMRAND_H
