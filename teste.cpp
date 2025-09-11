#include "./include/random/xoshiro.h" // Your custom xoshiro header

#include <iostream>
#include <vector>
#include <numeric>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <string>

inline thread_local xso::rng gen;

// --- Method 1: The fast, arithmetic-based approach ---
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

// --- Method 2: The standard library's approach (our baseline for quality) ---
inline double random_real_stdlib(double lower, double upper) {
    std::uniform_real_distribution<double> dis(lower, upper);
    return dis(gen);
}
inline int random_integer_stdlib(int lower, int upper) {
    std::uniform_int_distribution<int> dis(lower, upper);
    return dis(gen);
}

/**
 * @brief Analyzes the distribution of random integers from a given generator function.
 */
void analyze_int_distribution(
    const std::string& title,
    int (*generator)(int, int),
    long long samples,
    int min_val,
    int max_val)
{
    std::cout << "--- Analyzing Distribution for: " << title << " ---\n";

    const int num_buckets = max_val - min_val + 1;
    std::vector<long long> counts(num_buckets, 0);

    for (long long i = 0; i < samples; ++i) {
        int value = generator(min_val, max_val);
        counts[value - min_val]++;
    }

    const double expected_count = static_cast<double>(samples) / num_buckets;
    double chi_squared_stat = 0.0;
    long long min_actual_count = samples;
    long long max_actual_count = 0;

    for (long long count : counts) {
        double diff = static_cast<double>(count) - expected_count;
        chi_squared_stat += diff * diff / expected_count;
        min_actual_count = std::min(min_actual_count, count);
        max_actual_count = std::max(max_actual_count, count);
    }

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Total Samples: " << samples << " | Range: [" << min_val << ", " << max_val << "]\n";
    std::cout << "Expected count per number: " << expected_count << "\n";
    std::cout << "Actual counts ranged from " << min_actual_count << " to " << max_actual_count << ".\n";
    std::cout << "Chi-Squared Statistic: " << chi_squared_stat << " (lower is better; ~" << num_buckets - 1 << ".0 is ideal)\n\n";
}

/**
 * @brief Analyzes the distribution of random real numbers from a given generator function.
 */
void analyze_real_distribution(
    const std::string& title,
    double (*generator)(double, double),
    long long samples,
    double min_val,
    double max_val,
    int num_buckets)
{
    std::cout << "--- Analyzing Distribution for: " << title << " ---\n";
    
    std::vector<long long> counts(num_buckets, 0);
    const double bucket_width = (max_val - min_val) / num_buckets;

    for (long long i = 0; i < samples; ++i) {
        double value = generator(min_val, max_val);
        int bucket_index = static_cast<int>((value - min_val) / bucket_width);
        // Clamp the index to handle the edge case where value == max_val
        bucket_index = std::min(bucket_index, num_buckets - 1);
        counts[bucket_index]++;
    }

    const double expected_count = static_cast<double>(samples) / num_buckets;
    double chi_squared_stat = 0.0;
    long long min_actual_count = samples;
    long long max_actual_count = 0;

    for (long long count : counts) {
        double diff = static_cast<double>(count) - expected_count;
        chi_squared_stat += diff * diff / expected_count;
        min_actual_count = std::min(min_actual_count, count);
        max_actual_count = std::max(max_actual_count, count);
    }

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Total Samples: " << samples << " | Range: [" << min_val << ", " << max_val << ") | Buckets: " << num_buckets << "\n";
    std::cout << "Expected count per bucket: " << expected_count << "\n";
    std::cout << "Actual counts ranged from " << min_actual_count << " to " << max_actual_count << ".\n";
    std::cout << "Chi-Squared Statistic: " << chi_squared_stat << " (lower is better; ~" << num_buckets - 1 << ".0 is ideal)\n\n";
}


int main() {
    constexpr long long SAMPLES = 1'000'000;
    
    // --- Integer Analysis ---
    constexpr int INT_MIN_VAL = 0;
    constexpr int INT_MAX_VAL = 99;
    std::cout << "====== Running Distribution Analysis for INTEGER Generators ======\n\n";
    analyze_int_distribution("Arithmetic Method (Modulo)", random_integer, SAMPLES, INT_MIN_VAL, INT_MAX_VAL);
    analyze_int_distribution("std::uniform_int_distribution", random_integer_stdlib, SAMPLES, INT_MIN_VAL, INT_MAX_VAL);

    // --- Real Number Analysis ---
    constexpr double REAL_MIN_VAL = 0.0;
    constexpr double REAL_MAX_VAL = 100.0;
    constexpr int REAL_BUCKETS = 100;
    std::cout << "\n====== Running Distribution Analysis for REAL Generators ======\n\n";
    analyze_real_distribution("Arithmetic Method (Scaling)", random_real, SAMPLES, REAL_MIN_VAL, REAL_MAX_VAL, REAL_BUCKETS);
    analyze_real_distribution("std::uniform_real_distribution", random_real_stdlib, SAMPLES, REAL_MIN_VAL, REAL_MAX_VAL, REAL_BUCKETS);

    return 0;
}



// ------------------- ARQUIVO CRIADO COM IA GENERATIVA PARA TESTE -------------------