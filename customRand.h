#include <random>
#ifndef CUSTOMRAND_H
#define CUSTOMRAND_H
std::mt19937 gen(std::random_device{}());

double random_real(double lower, double upper) {
    static std::uniform_real_distribution<> dis;
    dis.param(std::uniform_real_distribution<>::param_type(lower, upper));
    return dis(gen);
}

int random_integer(int lower, int upper){
    static std::uniform_int_distribution<> dis;
    dis.param(std::uniform_int_distribution<>::param_type(lower, upper));
    return dis(gen);
}
#endif