#include <tools/customRand.h>
#include <tools/MathArray.h>
#include <tools/Square.h>
#include <algorithm>
#include <random>
#include <iostream>

using namespace std;
typedef float number;

const size_t POPULATION_SIZE = 50;
const size_t GENE_SIZE = 17;
const size_t INTERSECTION_PENALTY = 100;
const size_t EMPTY_PENALTY = 5;
const size_t ELITE_COUNT = 3;     
const size_t RANDOM_COUNT = 10;

const number MUTATION_RATE = 0.5;
const number MUTATION_STRENGTH = 0.2;

bool isSquareValid(const Square& s, number L) {
    auto vertices = s.getVertices(); 
    for (const auto& v : vertices) {
        if (v.x < 0 || v.x > L || v.y < 0 || v.y > L) {
            return false; 
        }
    }
    return true; 
}

class Gene{
    public:
        MathArray<Square,GENE_SIZE> data;
        number L = 4.75;
        void initialize(){
            static_assert(GENE_SIZE >= 4, "GENE_SIZE must be 4 or greater for this optimization.");
            const number half_len = 0.5;
            data[0] = Square(Point(half_len, half_len), 0.0, 1.0);         
            data[1] = Square(Point(L - half_len, half_len), 0.0, 1.0);     
            data[2] = Square(Point(half_len, L - half_len), 0.0, 1.0);    
            data[3] = Square(Point(L - half_len, L - half_len), 0.0, 1.0); 
            for(size_t i=4;i<GENE_SIZE;i++){
                const number square_len = 0.5;
                number x = random_real(square_len,L-square_len);
                number y = random_real(square_len,L-square_len);
                number closest_dist = min({x,L-x,y,L-y});
                const number sqrt_2 = sqrt(2.0);
                number theta;

                if (closest_dist >= 1.0 / sqrt_2) {
                    theta = random_real(0, PI / 2.0);
                }
                else {
                    number theta_bound = acos(sqrt_2 * closest_dist);
                    number range1_end = PI / 4.0 - theta_bound;   
                    number range2_start = PI / 4.0 + theta_bound; 

                    if (random_integer(0, 1) == 0) {
                        theta = random_real(0, range1_end);
                    } else {
                        theta = random_real(range2_start, PI / 2.0);
                    }
                }      
                data[i] = Square(Point(x,y),theta,1);
            }
        }
        number fitness(){
            number intersectionArea = 0.0;
            for(size_t i=0;i<GENE_SIZE;++i){
                for(size_t j=i+1;j<GENE_SIZE;++j){
                    intersectionArea += areaOfSquareIntersections(data[i],data[j]);
                }
            }
            number min_x = L, max_x = 0.0, min_y = L, max_y = 0.0;
            for (const auto& square : data) {
                auto vertices = square.getVertices();
                for (const auto& v : vertices) {
                    min_x = min(min_x, v.x);
                    max_x = max(max_x, v.x);
                    min_y = min(min_y, v.y);
                    max_y = max(max_y, v.y);
                }
            }
            number boundingBoxArea = (max_x - min_x) * (max_y - min_y);
            return -boundingBoxArea * EMPTY_PENALTY - intersectionArea * INTERSECTION_PENALTY;  
        }
        void mutate() {
            normal_distribution<> d(0, MUTATION_STRENGTH);
            for (size_t i=4; i<GENE_SIZE; ++i) {
                Square& square = data[i]; 
                if (random_real(0, 1) < MUTATION_RATE) {
                    Square potential_square = square; 
                    potential_square.c.x += d(gen);   

                    if (isSquareValid(potential_square, L)) {
                        square = potential_square;
                    }
                }

                if (random_real(0, 1) < MUTATION_RATE) {
                    Square potential_square = square;
                    potential_square.c.y += d(gen);   

                    if (isSquareValid(potential_square, L)) {
                        square = potential_square;
                    }
                }

                if (random_real(0, 1) < MUTATION_RATE) {
                    Square potential_square = square;
                    potential_square.t += d(gen);
                    potential_square.t = fmod(potential_square.t, PI / 2.0);
                    if (potential_square.t < 0) {
                        potential_square.t += PI / 2.0;
                    }

                    if (isSquareValid(potential_square, L)) {
                        square = potential_square;
                    }
                }
            }
        }
        Gene cross(const Gene& partner) const {
            Gene child;
            child.L = this->L;

            for (size_t i = 0; i < 4; ++i) {
                child.data[i] = this->data[i];
            }

            for (size_t i = 4; i < GENE_SIZE; ++i) {
                if (random_real(0, 1) < 0.05) {
                    const Square& s1 = this->data[i];
                    const Square& s2 = partner.data[i];

                    Point avg_center((s1.c.x + s2.c.x) / 2.0, (s1.c.y + s2.c.y) / 2.0);
                    number avg_theta = (s1.t + s2.t) / 2.0;

                    Square potential_child_square(avg_center, avg_theta, 1.0);

                    if (isSquareValid(potential_child_square, this->L)) {
                        child.data[i] = potential_child_square;
                    } else {
                        child.data[i] = (random_real(0, 1) < 0.5) ? s1 : s2;
                    }
                } else {
                    if (random_real(0, 1) < 0.5) {
                        child.data[i] = this->data[i];
                    } else {
                        child.data[i] = partner.data[i];
                    }
                }
            }
            return child;
        }
};

bool orderByFitness(Gene& a, Gene& b){
    return a.fitness() > b.fitness();
}

Gene tournament_selection(MathArray<Gene, POPULATION_SIZE>& population) {
    const size_t TOURNAMENT_SIZE = 3;
    size_t best_index = random_integer(0, POPULATION_SIZE - 1);

    for (size_t i = 1; i < TOURNAMENT_SIZE; ++i) {
        size_t contestant_index = random_integer(0, POPULATION_SIZE - 1);
        if (orderByFitness(population[contestant_index], population[best_index])) {
            best_index = contestant_index;
        }
    }
    return population[best_index];
}
MathArray<Gene, POPULATION_SIZE> population;
Gene best;

void initializePopulation(){
    for (size_t i = 0; i < POPULATION_SIZE; ++i) {
        population[i].initialize();
    }
}

MathArray<Gene,POPULATION_SIZE> evolve_once(size_t generation_number = 0){
    sort(population.begin(), population.end(), orderByFitness);

    if (orderByFitness(population[0], best)) {
        best = population[0];
    }

    MathArray<Gene, POPULATION_SIZE> new_population;
    const size_t ELITE_COUNT = 2; 

    for (size_t i = 0; i < ELITE_COUNT; ++i) {
        new_population[i] = population[i];
    }

    for (size_t i = ELITE_COUNT; i < POPULATION_SIZE; ++i) {
        Gene parent1 = tournament_selection(population);
        Gene parent2 = tournament_selection(population);
        Gene child = parent1.cross(parent2);
        child.mutate(); 
        new_population[i] = child;
    }
    population = new_population;
    cout << "L=" << population[0].L << '\n' << "fitness = " << population[0].fitness() << '\n';
    for(const Square& sq : population[0].data){
        printf("(%f, %f, %f, 1.0)\n",sq.c.x, sq.c.y, sq.t);
    }
    return population;
}
Gene evolve(const size_t number_of_generations){
    
    for (size_t i = 0; i < POPULATION_SIZE; ++i) {
        population[i].initialize();
    }
    for(int i=0;i<100;i++){
        evolve_once();
    }

    return best; 
}