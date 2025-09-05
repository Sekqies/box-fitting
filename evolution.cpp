#include "customRand.h"
#include "MathArray.h"
#include "Square.h"
#include <algorithm>
#include <random>
#include <iostream>

using namespace std;

const size_t POPULATION_SIZE = 100;
const size_t GENE_SIZE = 17;
const size_t INTERSECTION_PENALTY = 10;
const size_t EMPTY_PENALTY = 5;

const double MUTATION_RATE = 0.50;
const double MUTATION_STRENGTH = 0.05;

bool isSquareValid(const Square& s, double L) {
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
        double L = 4.75;
        void initialize(){
            static_assert(GENE_SIZE >= 4, "GENE_SIZE must be 4 or greater for this optimization.");
            const double half_len = 0.5;
            data[0] = Square(Point(half_len, half_len), 0.0, 1.0);         
            data[1] = Square(Point(L - half_len, half_len), 0.0, 1.0);     
            data[2] = Square(Point(half_len, L - half_len), 0.0, 1.0);    
            data[3] = Square(Point(L - half_len, L - half_len), 0.0, 1.0); 
            for(size_t i=4;i<GENE_SIZE;i++){
                const double square_len = 0.5;
                double x = random_real(square_len,L-square_len);
                double y = random_real(square_len,L-square_len);
                double closest_dist = min({x,L-x,y,L-y});
                const double sqrt_2 = sqrt(2.0);
                double theta;

                if (closest_dist >= 1.0 / sqrt_2) {
                    theta = random_real(0, PI / 2.0);
                }
                else {
                    double theta_bound = acos(sqrt_2 * closest_dist);
                    double range1_end = PI / 4.0 - theta_bound;   
                    double range2_start = PI / 4.0 + theta_bound; 

                    if (random_integer(0, 1) == 0) {
                        theta = random_real(0, range1_end);
                    } else {
                        theta = random_real(range2_start, PI / 2.0);
                    }
                }      
                data[i] = Square(Point(x,y),theta,1);
            }
        }
        double fitness(){
            double intersectionArea = 0.0;
            for(size_t i=0;i<GENE_SIZE;++i){
                for(size_t j=i+1;j<GENE_SIZE;++j){
                    intersectionArea += areaOfSquareIntersections(data[i],data[j]);
                }
            }
            double unused_area = L*L - GENE_SIZE + intersectionArea;
            return -unused_area - intersectionArea;  
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
        Gene cross(Gene& partner) const{
            Gene child;
            child.L = this->L; 
            for (size_t i = 0; i < 4; ++i) {
                child.data[i] = this->data[i];
            }
            for (size_t i = 4; i < GENE_SIZE; ++i) {
                if (random_real(0, 1) < 0.5) {
                    child.data[i] = this->data[i]; 
                } else {
                    child.data[i] = partner.data[i];
                }
            }
            return child;
        }
};

bool orderByFitness(Gene& a, Gene& b){
    return a.fitness() > b.fitness();
}

Gene tournament_selection(MathArray<Gene, POPULATION_SIZE>& population) {
    const size_t TOURNAMENT_SIZE = 5;
    size_t best_index = random_integer(0, POPULATION_SIZE - 1);

    for (size_t i = 1; i < TOURNAMENT_SIZE; ++i) {
        size_t contestant_index = random_integer(0, POPULATION_SIZE - 1);
        if (orderByFitness(population[contestant_index], population[best_index])) {
            best_index = contestant_index;
        }
    }
    return population[best_index];
}

Gene evolve(const size_t number_of_generations){
    MathArray<Gene, POPULATION_SIZE> population;
    for (size_t i = 0; i < POPULATION_SIZE; ++i) {
        population[i].initialize();
    }

    Gene best = population[0];

    for (size_t gen = 0; gen < number_of_generations; ++gen) {
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
        cout << "Generation " << gen+1 << "best: \n";
        cout << "L=" << best.L << '\n' << "fitness = " << best.fitness() << '\n';
        for(const Square& sq : best.data){
            printf("(%f, %f, %f)\n",sq.c.x, sq.c.y, sq.t);
        }
    }
    return best; 
}


int main(){
    evolve(100);
    return 0;
}