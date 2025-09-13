#include <sstream>
#include <fstream>
#include <vector>


using std::string;
using std::stringstream;
using std::vector;
using std::pair;

class GenerationData{
    public:
    double maximumFitness;
    double averageFitness;
    size_t generationNumber = 0;
    GenerationData(double maximumFitness, double averageFitness, size_t generationNumber){
        this->maximumFitness = maximumFitness;
        this->averageFitness = averageFitness;
        this->generationNumber = generationNumber;
    }
};

class EvolutionData{
    public:
    vector<GenerationData> data;    
    bool wrote = false;
    EvolutionData(){
        data.reserve(100);
    }
    void pushGeneration(pair<Gene,double> generation_data, size_t generation_number){
        data.push_back(GenerationData(generation_data.first.fitness,generation_data.second,generation_number));
    }
    void write(const string filename){
        wrote = true;
        ofstream file(filename);
        sort(data.begin(),data.end(),[](GenerationData a, GenerationData b){
            return a.generationNumber < b.generationNumber;
        });
        file << "# Generation MaxFitness AvgFitness\n";
        for(const GenerationData& gd : data){
            if(gd.generationNumber > 0) {
                file << gd.generationNumber << " "
                     << gd.maximumFitness << " "
                     << gd.averageFitness << "\n";
            }
        }
    }
    ~EvolutionData(){
        if(!wrote) write("generation_data.dat");
    }


};

