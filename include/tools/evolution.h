#ifndef EVOLUTION_H
#define EVOLUTION_H

#include <iostream>
#include <vector>
#include <tools/customRand.h>
#include <tools/MathArray.h>
#include <tools/Square.h>
#include <utility>
#include <algorithm>
#include <limits>
#include <cmath>
#include <thread>
#include <mutex>
#include <stdexcept>
#include <random> 

using std::vector;
using std::sort;
using std::shuffle;
using std::max;
using std::min;
using std::round;

typedef float number;

// --- GA Configuration ---
constexpr int GENE_SIZE = 17; // N: Number of squares to pack
constexpr number SQUARE_SIDE_LENGTH = 1.0; // Side length of small squares
constexpr number BOX_SIDE_LENGTH = 5;   // L: Side length of the container

constexpr int POPULATION_SIZE = 150;
constexpr double ELITISM_RATE = 0.1; // 10% of the best individuals are carried over
constexpr double MUTATION_RATE = 0.05; // 5% chance per square to mutate
constexpr int TOURNAMENT_SIZE = 5;

constexpr double ROTATIONAL_SNAP_PROBABILITY = 0.5; // Chance to rotate and move a square that's near an edge 
constexpr double PREDATION_RATE = 0.1; // Percentage of non-elites to be culled each generation
constexpr double DISASTER_PROBABILITY = 0.02; // Probability of a disaster event in any given generation
constexpr double DISASTER_HYPERMUTATION_RATE = 0.50; // The higher mutation rate used during a disaster

constexpr double BOUNDARY_PENALTY_WEIGHT = 0.5; // How much to penalize non-alignment (DEPRECATED)
constexpr double OUT_OF_BOUNDS_WEIGHT = 300; // How much to penalize for squares out of bounds
constexpr double OVERLAP_WEIGHT = 5; // How much to penalize for squares overlapping



extern thread_local xso::rng gen;

class Gene { // Gene is a set of squares
public:
    MathArray<Square, GENE_SIZE> data;
    double fitness;

    Gene() : fitness(std::numeric_limits<double>::max()) {
        for (size_t i = 0; i < GENE_SIZE; ++i) {
            data[i] = Square(
                Point(random_real(0, BOX_SIDE_LENGTH), random_real(0, BOX_SIDE_LENGTH)),
                random_real(0, 2 * M_PI),
                SQUARE_SIDE_LENGTH
            );
        }
    }

    void calculateFitness() {
        double overlap_penalty = 0.0;
        double bounds_penalty = 0.0;
        
        const Square container_box(Point(BOX_SIDE_LENGTH / 2.0, BOX_SIDE_LENGTH / 2.0), 0, BOX_SIDE_LENGTH);
        const Point box_center(BOX_SIDE_LENGTH / 2.0, BOX_SIDE_LENGTH / 2.0);
        for (size_t i = 0; i < GENE_SIZE; ++i) {
            // Overlap with other squares
            for (size_t j = i + 1; j < GENE_SIZE; ++j) {
                overlap_penalty += areaOfSquareIntersections(data[i], data[j]);
            }
            // Penalty for being outside the container
            // This should be avoided by the mutate() functions, but this works as a last fallback
            number intersection_with_box = areaOfSquareIntersections(data[i], container_box);
            number square_area = data[i].l * data[i].l;
            bounds_penalty += (square_area - intersection_with_box);
        }
        fitness = (overlap_penalty * OVERLAP_WEIGHT) + bounds_penalty * OUT_OF_BOUNDS_WEIGHT;
    }
};

// --- Genetic Algorithm Functions ---

Gene cross(const Gene& parent1, const Gene& parent2) {
    Gene child;
    for (size_t i = 0; i < GENE_SIZE; ++i) {
        child.data[i] = (random_real(0, 1) < 0.5) ? parent1.data[i] : parent2.data[i];
    }
    return child;
}

// Creates a gene with squares arranged in a grid.
// This is the obvious, trivial solution for any grid with L^2 > NUMBER_SQUARES
Gene createGridGene() {
    Gene gridGene;
    int grid_dim = ceil(sqrt(GENE_SIZE)); 
    if(grid_dim * SQUARE_SIDE_LENGTH > BOX_SIDE_LENGTH){
        return Gene(); // Return a random gene if the grid doesn't fit
    }
    const number spacing = SQUARE_SIDE_LENGTH;
    number grid_total_size = grid_dim * spacing;
    number start_offset = max(0.0, (double)(BOX_SIDE_LENGTH - grid_total_size) / 2.0);
    number center_offset = spacing / 2.0;

    int square_index = 0;
    for (int i = 0; i < grid_dim && square_index < GENE_SIZE; ++i) {
        for (int j = 0; j < grid_dim && square_index < GENE_SIZE; ++j) {
            number pos_x = start_offset + (j * spacing) + center_offset;
            number pos_y = start_offset + (i * spacing) + center_offset;
            gridGene.data[square_index].c = Point(pos_x, pos_y);
            gridGene.data[square_index].t = 0.0;
            square_index++;
        }
    }
    return gridGene;
}

vector<Gene> initializeGenes() {
    vector<Gene> population;
    population.reserve(POPULATION_SIZE);
    population.push_back(createGridGene());
    
    for (int i = 1; i < POPULATION_SIZE; ++i) {
        population.push_back(Gene());
    }
    for (auto& gene : population) {
        gene.calculateFitness();
    }
    sort(population.begin(), population.end(), [](const Gene& a, const Gene& b) {
        return a.fitness < b.fitness;
    });

    return population;
}

// Standalone mutation function to be called from threads. 

void mutate_gene(Gene& gene, double rate) {
    for (size_t j = 0; j < GENE_SIZE; ++j) {
        if (random_real(0, 1) < rate) {
            int mutation_type = random_integer(0, 2);
            switch(mutation_type) {
                case 0: // Nudge position
                    gene.data[j].c.x += random_real(-0.1 * BOX_SIDE_LENGTH, 0.1 * BOX_SIDE_LENGTH);
                    gene.data[j].c.y += random_real(-0.1 * BOX_SIDE_LENGTH, 0.1 * BOX_SIDE_LENGTH);
                    break;
                case 1: // Jump to a new position
                    gene.data[j].c.x = random_real(0, BOX_SIDE_LENGTH);
                    gene.data[j].c.y = random_real(0, BOX_SIDE_LENGTH);
                    break;
                case 2: // Change rotation
                    if (random_real(0, 1) < ROTATIONAL_SNAP_PROBABILITY) {
                        gene.data[j].t = round(gene.data[j].t / (M_PI / 2.0)) * (M_PI / 2.0);
                    } else {
                        gene.data[j].t = random_real(0, 2 * M_PI);
                    }
                    break;
            }
            // Clamp coordinates to stay within bounds
            gene.data[j].c.x = max((number)0.0, min(BOX_SIDE_LENGTH, gene.data[j].c.x));
            gene.data[j].c.y = max((number)0.0, min(BOX_SIDE_LENGTH, gene.data[j].c.y));
        }
    }
}

// Tournament selection that operates on a provided parent pool.
const Gene& tournament_selection(const vector<Gene>& parent_pool) {
    if (parent_pool.empty()) {
        //This should never happen, since it implies a predation rate of 100 and elitism of 0. 
        throw std::runtime_error("Parent pool for tournament selection is empty!");
    }
    
    int best_index = -1;
    double best_fitness = std::numeric_limits<double>::max();

    for (int i = 0; i < TOURNAMENT_SIZE; ++i) {
        int random_index = random_integer(0, parent_pool.size() - 1);
        if (parent_pool[random_index].fitness < best_fitness) {
            best_fitness = parent_pool[random_index].fitness;
            best_index = random_index;
        }
    }
    return parent_pool[best_index];
}

vector<Gene> evolve_generation(const vector<Gene>& current_population, const unsigned int NUM_THREADS) {
    //Elitism and predation
    vector<Gene> survivor_pool;
    survivor_pool.reserve(POPULATION_SIZE);

    int elite_count = static_cast<int>(POPULATION_SIZE * ELITISM_RATE);
    for (int i = 0; i < elite_count; ++i) {
        survivor_pool.push_back(current_population[i]);
    }

    vector<int> non_elite_indices;
    for (size_t i = elite_count; i < current_population.size(); ++i) {
        non_elite_indices.push_back(i);
    }
    shuffle(non_elite_indices.begin(), non_elite_indices.end(), gen);

    int predation_kill_count = static_cast<int>(non_elite_indices.size() * PREDATION_RATE); 
    int non_elite_survivor_count = non_elite_indices.size() - predation_kill_count; 
    // Random number of non-elites get eliminated

    for (int i = 0; i < non_elite_survivor_count; ++i) {
        survivor_pool.push_back(current_population[non_elite_indices[i]]);
    }

    // Mutation and crossover
    double current_mutation_rate = MUTATION_RATE;
    if (random_real(0, 1) < DISASTER_PROBABILITY) {
        current_mutation_rate = DISASTER_HYPERMUTATION_RATE;
    }

    vector<Gene> new_population = survivor_pool;
    int offspring_needed = POPULATION_SIZE - survivor_pool.size();
    
    if (offspring_needed > 0) {
        vector<std::thread> workers;
        std::mutex new_population_mutex;
        size_t chunk_size = offspring_needed / NUM_THREADS;

        for (unsigned int i = 0; i < NUM_THREADS; ++i) {
            size_t num_to_create = (i == NUM_THREADS - 1) ? (offspring_needed - (i * chunk_size)) : chunk_size;
            
            workers.emplace_back([&, num_to_create, current_mutation_rate]() {
                vector<Gene> offspring_batch;
                offspring_batch.reserve(num_to_create);
                for (size_t j = 0; j < num_to_create; ++j) {
                    const Gene& parent1 = tournament_selection(survivor_pool);
                    const Gene& parent2 = tournament_selection(survivor_pool);
                    Gene child = cross(parent1, parent2);
                    mutate_gene(child, current_mutation_rate);
                    offspring_batch.push_back(child);
                }
                
                std::lock_guard<std::mutex> guard(new_population_mutex);
                new_population.insert(new_population.end(), offspring_batch.begin(), offspring_batch.end());
            });
        }
        for (auto& worker : workers) { worker.join(); }
    }
    {
        vector<std::thread> workers;
        size_t chunk_size = new_population.size() / NUM_THREADS;
        for (unsigned int i = 0; i < NUM_THREADS; ++i) {
            size_t start = i * chunk_size;
            size_t end = (i == NUM_THREADS - 1) ? new_population.size() : start + chunk_size;
            workers.emplace_back([&new_population, start, end]() {
                for (size_t j = start; j < end; ++j) {
                    new_population[j].calculateFitness();
                }
            });
        }
        for (auto& worker : workers) { worker.join(); }
    }
    
    sort(new_population.begin(), new_population.end(), [](const Gene& a, const Gene& b) {
        return a.fitness < b.fitness;
    });

    return new_population;
}

#endif // EVOLUTION_H