//
// Created by srikanth on 6/13/17.
//

#pragma once
#include <math.h>
#include <iostream>
#include <algorithm>
#include "speck_64_128_twostage.cpp"


double_t integration_precision = 0.001;

double_t integrate(double_t (*f)(double_t, uint64_t, uint64_t), double_t l1, double_t l2, uint64_t b, uint64_t r){
    double_t area = 0.0, x = l1;
    double_t p = integration_precision;
    while(x<l2){
        area += f(x+0.5*p, b, r)*p;
        x += p;
    }
    return area;
}

double_t false_positive_probability(double_t threshold, uint64_t b, uint64_t r){
    double_t (*probability)(double_t, uint64_t, uint64_t) = [](double_t s, uint64_t b, uint64_t r){ return 1 - pow((1 - pow(s, (double)r)), (double)b);};
    return integrate(probability, 0.0, threshold, b, r);
}

double_t false_negative_probability(double_t threshold, uint64_t b, uint64_t r){
    double_t (*probability)(double_t, uint64_t, uint64_t) = [](double_t s,  uint64_t b, uint64_t r){ return 1 - (1 - pow((1 - pow(s, (double)r)), (double)b));};
    return integrate(probability, threshold, 1, b, r);
}

auto optimal_param(double_t threshold, uint64_t n_perms, double_t false_positive_weight, double false_negative_weight){
    double_t min_err = std::numeric_limits<double>::infinity();
    std::pair<uint64_t, uint64_t> param;

    for(uint64_t b = 1; b <= n_perms; b++){
        uint64_t max_r = n_perms / b;
        for(uint64_t r=1; r<=max_r; r++){
            double_t fpp = false_positive_probability(threshold, b, r);
            double_t fnp = false_negative_probability(threshold, b, r);
            double_t error = fpp*false_positive_weight + fnp*false_negative_weight;
            if(error<min_err){
                param.first = b;
                param.second = r;
                min_err = error;
            }
        }
    }
    return param;
}

auto optimal_threshold1(uint64_t fixed_key_size, uint64_t w_size, double threshold, double t_limit){
    std::cout << " KS: " << fixed_key_size << " WS: " << w_size << " Th: " << threshold << " Tl: " << t_limit << std::endl;
    uint64_t tokens_size = fixed_key_size - w_size + 1;
    double number_of_distinct_items_estimate = t_limit*w_size;
    double threshold_estimate = (float)(tokens_size - number_of_distinct_items_estimate)/(float)(tokens_size + number_of_distinct_items_estimate);
    double reductionFactor = 0.95;
    return reductionFactor*threshold_estimate;
}

auto optimal_threshold(uint64_t fixed_key_size, uint64_t w_size, double threshold, double t_limit){
    std::cout << " KS: " << fixed_key_size << " WS: " << w_size << " Th: " << threshold << " Tl: " << t_limit << std::endl;
    uint64_t tokens_size = fixed_key_size - w_size + 1;
    double number_of_distinct_items_estimate = w_size + t_limit - 1;
    double threshold_estimate = (float)(tokens_size - number_of_distinct_items_estimate)/(float)(tokens_size);
    double reductionFactor = 0.90;
    return reductionFactor*threshold_estimate;
}

uint64_t remap64(uint64_t hash, uint64_t a, uint64_t b){
    //uint32_t plaintext[2] = {0x7475432dUL, static_cast<uint32_t>()};
    uint64_t key[2] = {a, b}, res=0ULL;
    uint32_t ciphertext[2];
    uint32_t key_schedule[ROUNDS];
    speck_setup((uint32_t*)&key, key_schedule);
    speck_encrypt((uint32_t*)&hash, key_schedule, ciphertext);
    res = ciphertext[0];
    res = res << 32;
    res |= ciphertext[1];
    return res;
}

void unique_vec(std::vector<uint64_t>& v){
    if(v.size()>0){
        std::sort(v.begin(), v.end());
        auto end = std::unique(v.begin(), v.end());
        v.resize(end-v.begin());
    }
}