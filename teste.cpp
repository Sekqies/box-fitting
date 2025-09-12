#include <iostream>
#include <sstream>
#include <vector>
#include "include\tools\Square.h"

using namespace std;

std::vector<Square> parseSquares(const std::string& inputStr) {
    std::vector<Square> squares;
    std::stringstream ss(inputStr); // Treat the entire input string as a stream
    std::string line;

    // Process the stream line by line
    while (std::getline(ss, line)) {
        // Use another stringstream to parse each individual line
        std::stringstream line_ss(line);

        double x, y, t;
        char open_paren, comma1, comma2, close_paren;

        // Try to extract data in the expected format: (x,y,t)
        // The ">>" operator will automatically skip whitespace.
        // If the format doesn't match, the extraction will fail, and the 'if' condition will be false.
        if (line_ss >> open_paren && open_paren == '(' &&
            line_ss >> x &&
            line_ss >> comma1 && comma1 == ',' &&
            line_ss >> y &&
            line_ss >> comma2 && comma2 == ',' &&
            line_ss >> t &&
            line_ss >> close_paren && close_paren == ')') {

            // If successful, create a Square object and add it to the vector
            squares.emplace_back(Point(x,y), t, 1);
        }
    }

    return squares;
}

void printSquares(vector<Square>& sqr){
    for(const Square& sq : sqr){
        printf("(%f %f %f)\n",sq.c.x,sq.c.y,sq.t);
    }
    printf("\n\n");
}

double calculateFitness(vector<Square>& data){
    const size_t GENE_SIZE = data.size();
    const double BOX_SIDE_LENGTH = 4.85;
    double overlap_penalty = 0.0;
    double bounds_penalty = 0.0;
    double positional_adherence_penalty = 0.0;

    const Point box_center(BOX_SIDE_LENGTH / 2.0, BOX_SIDE_LENGTH / 2.0);
    const Square container_box(box_center, 0, BOX_SIDE_LENGTH);

    for (size_t i = 0; i < GENE_SIZE; ++i) {
        for (size_t j = i + 1; j < GENE_SIZE; ++j) {
            overlap_penalty += areaOfSquareIntersections(data[i], data[j]);
        }
        // Penalty for being outside the container
        number intersection_with_box = areaOfSquareIntersections(data[i], container_box);
        number square_area = data[i].l * data[i].l;
        bounds_penalty += (square_area - intersection_with_box);
    }
    return (overlap_penalty * 5) + bounds_penalty * 300;
}


int main(){
    vector<Square> sqs = parseSquares(R"(
(1.580747,3.353917,0.875791)
(4.325975,0.949594,0.000000)
(0.611020,1.109277,6.051540)
(0.729663,4.148878,0.822272)
(4.219793,2.042378,1.388005)
(2.263891,4.105357,2.419573)
(1.623305,1.806468,5.496466)
(0.520240,3.337980,4.712389)
(4.135303,3.784060,4.432355)
(1.513235,0.624713,2.958930)
(2.952622,3.767261,6.283185)
(0.538379,2.335698,1.570796)
(1.653581,2.492916,4.712389)
(2.554513,1.305122,3.141593)
(2.836719,2.918109,2.301713)
(3.313730,2.384610,1.570796)
(3.028640,1.007975,2.257814))");
    printSquares(sqs);
    const size_t n = sqs.size();
    double intersection_amount = 0.0;
    for(int i=0;i<n;i++){
        for(int j=i+1;j<n;j++){
            intersection_amount += areaOfSquareIntersections(sqs[i],sqs[j]);
        }
    }
    cout << intersection_amount;
    cout << "\n Fitness: " << calculateFitness(sqs);

}