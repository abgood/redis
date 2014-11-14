#include "redis.h"

static uint32_t dict_hash_function_seed = 5381;

// 设置字典hash函数种子
void dictSetHashFunctionSeed(uint32_t seed) {
    dict_hash_function_seed = seed;
}
