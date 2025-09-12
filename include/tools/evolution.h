#include <iostream>
#include <vector>
#include <tools/customRand.h>
#include <tools/MathArray.h>
#include <tools/Square.h>
#include <utility>

typedef float number;
// --- GA Configuration ---
constexpr int GENE_SIZE = 17; // N: Number of suares to pack
constexpr number SQUARE_SIDE_LENGTH = 1.0; // Side length of small squares
constexpr number BOX_SIDE_LENGTH = 4.8;   // L: Side length of the container

constexpr int POPULATION_SIZE = 150;
constexpr double ELITISM_RATE = 0.1; // 10% of the best individuals are carried over
constexpr double MUTATION_RATE = 0.05; // 5% chance per square to mutate
constexpr double IMMIGRATION_RATE = 0.05; // 5% of new individuals are random
constexpr int TOURNAMENT_SIZE = 5; 

constexpr double ROTATIONAL_SNAP_PROBABILITY = 0.5; // Chance to rotate and move a square that's near an edge
constexpr number BOUNDARY_THRESHOLD = 1.5 * SQUARE_SIDE_LENGTH; // How close to an edge to be affected
constexpr double BOUNDARY_PENALTY_WEIGHT = 0.5; // How much to penalize non-alignment

// Percentage of non-elites to be culled each generation
constexpr double PREDATION_RATE = 0.1; 
// Probability of a disaster event in any given generation
constexpr double DISASTER_PROBABILITY = 0.02; 
// The higher mutation rate used during a disaster
constexpr double DISASTER_HYPERMUTATION_RATE = 0.50; 
extern thread_local xso::rng gen;


class Gene {
public:
    MathArray<Square, GENE_SIZE> data;
    double fitness;

    Gene() : fitness(std::numeric_limits<double>::max()) {
        for (size_t i = 0; i < GENE_SIZE; ++i) {
            data[i] = Square(
                Point(random_real(0, BOX_SIDE_LENGTH), random_real(0, BOX_SIDE_LENGTH)),
                random_real(0, 2 * PI),
                SQUARE_SIDE_LENGTH
            );
        }
    }

void calculateFitness() {
    double overlap_penalty = 0.0;
    double bounds_penalty = 0.0;
    double positional_adherence_penalty = 0.0;

    const Square container_box(Point(BOX_SIDE_LENGTH / 2.0, BOX_SIDE_LENGTH / 2.0), 0, BOX_SIDE_LENGTH);
    const Point box_center(BOX_SIDE_LENGTH / 2.0, BOX_SIDE_LENGTH / 2.0);

    const double max_dist = sqrt(pow(box_center.x, 2) + pow(box_center.y, 2));

    for (size_t i = 0; i < GENE_SIZE; ++i) {
        for (size_t j = i + 1; j < GENE_SIZE; ++j) {
            overlap_penalty += areaOfSquareIntersections(data[i], data[j]);
        }
        number intersection_with_box = areaOfSquareIntersections(data[i], container_box);
        number square_area = data[i].l * data[i].l;
        bounds_penalty += (square_area - intersection_with_box);
    }
    
    for (size_t i = 0; i < GENE_SIZE; ++i) {
        const auto& sq = data[i];

        // 1. Calculate the square's distance from the center of the box
        double dist_from_center = sqrt(pow(sq.c.x - box_center.x, 2) + pow(sq.c.y - box_center.y, 2));

        // 2. Normalize the distance to a multiplier between 0 and 1
        double positional_multiplier = dist_from_center / max_dist;

        // 3. Calculate the rotation penalty and scale it by the multiplier
        double rotation_penalty = abs(sin(2 * sq.t));
        positional_adherence_penalty += rotation_penalty * positional_multiplier;
    }

    // Add the weighted new penalty to the total fitness
    fitness = overlap_penalty * 10 + bounds_penalty + (positional_adherence_penalty * BOUNDARY_PENALTY_WEIGHT);
}

    void mutate() {
    for (size_t i = 0; i < GENE_SIZE; ++i) {
        if (random_real(0, 1) < MUTATION_RATE) {
            int mutation_type = random_integer(0, 2);
            switch(mutation_type) {
                case 0: // Nudge position
                    data[i].c.x += random_real(-0.1 * BOX_SIDE_LENGTH, 0.1 * BOX_SIDE_LENGTH);
                    data[i].c.y += random_real(-0.1 * BOX_SIDE_LENGTH, 0.1 * BOX_SIDE_LENGTH);
                    break;
                case 1: // Jump to a new position
                    data[i].c.x = random_real(0, BOX_SIDE_LENGTH);
                    data[i].c.y = random_real(0, BOX_SIDE_LENGTH);
                    break;
                case 2: // Change rotation 
                    if (random_real(0, 1) < ROTATIONAL_SNAP_PROBABILITY) {
                        // Snap to the nearest 90-degree angle
                        data[i].t = round(data[i].t / (PI / 2.0)) * (PI / 2.0);
                    } else {
                        // Regular random rotation
                        data[i].t = random_real(0, 2 * PI);
                    }
                    break;
            }
            // Clamp coordinates to stay within the box 
            data[i].c.x = std::max((number)0.0, std::min(BOX_SIDE_LENGTH, data[i].c.x));
            data[i].c.y = std::max((number)0.0, std::min(BOX_SIDE_LENGTH, data[i].c.y));
        }
    }
}
};

// --- Genetic Algorithm Functions ---

vector<Gene> population;

// Crossover 
Gene cross(const Gene& parent1, const Gene& parent2) {
    Gene child;
    for (size_t i = 0; i < GENE_SIZE; ++i) {
        child.data[i] = (random_real(0, 1) < 0.5) ? parent1.data[i] : parent2.data[i];
    }
    return child;
}

// Tournament selection
const Gene& tournament_selection() {
    int best_index = -1;
    double best_fitness = std::numeric_limits<double>::max();
    
    for (int i = 0; i < TOURNAMENT_SIZE; ++i) {
        int random_index = random_integer(0, POPULATION_SIZE - 1);
        if (population[random_index].fitness < best_fitness) {
            best_fitness = population[random_index].fitness;
            best_index = random_index;
        }
    }
    return population[best_index];
}

// Some scenarios have obvious solutions (e.g: packing 4 squares in a 2x2 grid). When it is possible to just make a grid, we do!
Gene createGridGene() {
    Gene gridGene;
    // This can be modified to int grid_dim = floor(sqrt(GENE_SIZE)); if we always want to make the closest grid possible
    // (This solution usually completely butchers any genetic variety)
    int grid_dim = ceil(sqrt(GENE_SIZE)); 
    if(grid_dim * grid_dim > BOX_SIDE_LENGTH * BOX_SIDE_LENGTH){
        return Gene();
    }
    const number spacing = SQUARE_SIDE_LENGTH;

    number grid_total_size = grid_dim * spacing;

    // Calculate the starting offset to center the grid within the box.
    // If the grid is somehow larger than the box, we start at 0.
    number start_offset = std::max((double)0.0, (BOX_SIDE_LENGTH - grid_total_size) / 2.0);
    
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

void initializeGenes() {
    population.clear();

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
}


std::pair<Gene,double> evolve_once() {
    double average = 0.0;
    vector<Gene> survivors;
    survivors.reserve(POPULATION_SIZE);

    // Elitism
    int elite_count = static_cast<int>(POPULATION_SIZE * ELITISM_RATE);
    for (int i = 0; i < elite_count; ++i) {
        survivors.push_back(population[i]);
    }

    // Random Predation
    vector<int> non_elite_indices;
    for (int i = elite_count; i < POPULATION_SIZE; ++i) {
        non_elite_indices.push_back(i);
    }
    shuffle(non_elite_indices.begin(), non_elite_indices.end(), gen);

    int predation_kill_count = static_cast<int>(non_elite_indices.size() * PREDATION_RATE);
    int non_elite_survivor_count = non_elite_indices.size() - predation_kill_count;
    
    for (int i = 0; i < non_elite_survivor_count; ++i) {
        survivors.push_back(population[non_elite_indices[i]]);
    }

    // Disaster
    double current_mutation_rate = MUTATION_RATE;
    if (random_real(0, 1) < DISASTER_PROBABILITY) {
        current_mutation_rate = DISASTER_HYPERMUTATION_RATE;
    }

    vector<Gene> new_population = survivors; 
    int offspring_needed = POPULATION_SIZE - survivors.size();

    // Custom tournament selection that works on the survivors list
    auto tournament_selection_on_survivors = [&]() -> const Gene& {
        int best_index = -1;
        double best_fitness = std::numeric_limits<double>::max();
        for (int i = 0; i < TOURNAMENT_SIZE; ++i) {
            int random_index = random_integer(0, survivors.size() - 1);
            if (survivors[random_index].fitness < best_fitness) {
                best_fitness = survivors[random_index].fitness;
                best_index = random_index;
            }
        }
        return survivors[best_index];
    };

    for (int i = 0; i < offspring_needed; ++i) {
        const Gene& parent1 = tournament_selection_on_survivors();
        const Gene& parent2 = tournament_selection_on_survivors();
        Gene child = cross(parent1, parent2);
        

        auto mutate_child = [&](Gene& gene, double rate) {
            for (size_t j = 0; j < GENE_SIZE; ++j) {
                if (random_real(0, 1) < rate) {
                    int mutation_type = random_integer(0, 2);
                    if (mutation_type == 0) {
                        gene.data[j].c.x += random_real(-0.1, 0.1);
                        gene.data[j].c.y += random_real(-0.1, 0.1);
                    } else if (mutation_type == 1) {
                        gene.data[j].c.x = random_real(0, BOX_SIDE_LENGTH);
                        gene.data[j].c.y = random_real(0, BOX_SIDE_LENGTH);
                    } else {
                        gene.data[j].t = random_real(0, 2 * PI);
                    }
                }
            }
        };
        mutate_child(child, current_mutation_rate);

        new_population.push_back(child);
    }
    
    population = new_population;

    // Return the best individual in the current population
    // The first one is not necessarily the best after breeding, so we re-calculate.
    // This also leaves the list sorted for future populations
    for (auto& gene : population) {
        gene.calculateFitness();
        average+= gene.fitness;
    }
    sort(population.begin(), population.end(), [](const Gene& a, const Gene& b) {
        return a.fitness < b.fitness;
    });
    average /= population.size();
    return std::make_pair(population[0],average);
}