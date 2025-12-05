#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <graphics/Shader.h>
#include <tools/evolution.h>
#include <tools/EvolutionData.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <mutex>
#include <atomic>
#include <numeric>

using namespace std;

const float SCREEN_SIZE = 800.00f;
const float PADDING = 50.0f;

// --- Thread variables ---
MathArray<Square,GENE_SIZE> shared_squares;
std::mutex data_mutex; 
std::atomic<bool> is_running = true; 
std::atomic<bool> is_rendering_enabled = true;
std::atomic<size_t> generation_number = 0;
EvolutionData evolutionData;
const unsigned int NUM_THREADS = std::thread::hardware_concurrency();

void evolution_worker() {
    vector<Gene> population = initializeGenes();

    while (is_running) {
        // Evolve the population for one generation
        population = evolve_generation(population, NUM_THREADS);
        
        // Lock the mutex to safely update the shared data for the rendering thread
        {
            std::lock_guard<std::mutex> guard(data_mutex);
            shared_squares = population[0].data; // population[0] is the best individual
            
            double average_fitness = 0.0;
            for(const Gene& g : population){
                average_fitness += g.fitness;
            }
            average_fitness /= population.size();
            
            evolutionData.pushGeneration(std::make_pair(population[0], average_fitness), generation_number);
        }
        generation_number++;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        is_rendering_enabled = !is_rendering_enabled;
        if (is_rendering_enabled) {
            std::cout << "Rendering ON" << std::endl;
        } else {
            std::cout << "Rendering OFF" << std::endl;
        }
    }
}

void drawPolygon(Shader& shader, unsigned int VAO, unsigned int VBO, const std::vector<Point>& vertices, glm::mat4& model) {
    if (vertices.size() < 2) {
        return;
    }

    shader.setMat4("model", model);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Point), vertices.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_LINE_LOOP, 0, vertices.size());
    glBindVertexArray(0);
}

int main() {

    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCREEN_SIZE, SCREEN_SIZE, "Visualização do algoritmo genético", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, SCREEN_SIZE, SCREEN_SIZE);

    Shader shader("./src/shaders/vertex.glsl","./src/shaders/fragment.glsl");

    unsigned int VAO, VBO;
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
    glEnableVertexAttribArray(0);

    std::thread worker(evolution_worker);
    size_t count = 0;
    size_t last_printed_generation = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (is_rendering_enabled) {
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            shader.use();

            glm::mat4 projection = glm::ortho(0.0f, SCREEN_SIZE, 0.0f, SCREEN_SIZE, -1.0f, 1.0f);
            shader.setMat4("projection", projection);
            glm::mat4 identity(1.0f);
            shader.setMat4("view", identity);
            size_t current_generation = generation_number.load();
            MathArray<Square, GENE_SIZE> squares_to_draw;
            {
                std::lock_guard<std::mutex> guard(data_mutex);
                squares_to_draw = shared_squares;
            } 
            if (!squares_to_draw.empty()) { 
                const float MAGNIFICATION = (SCREEN_SIZE - PADDING * 2) / BOX_SIDE_LENGTH;
                bool print = false;
                if(last_printed_generation != current_generation && current_generation % 100 == 0){
                    printf("\n\n\n");
                    print = true;
                }
                for(const Square& sq : squares_to_draw){
                    if(print){
                        printf("(%f,%f,%f)\n",sq.c.x,sq.c.y,sq.t);
                    }
                    glm::mat4 model = glm::mat4(1.0f);
                    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(MAGNIFICATION, MAGNIFICATION, 1.0f));
                    glm::mat4 translateMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(PADDING, PADDING, 0.0f));

                    model = translateMatrix * scaleMatrix;

                    drawPolygon(shader, VAO, VBO, sq.getVertices(), model);
                }
            }
            
            glfwSwapBuffers(window);
        }
    }
    
    is_running = false;
    worker.join();
    glfwTerminate();
    evolutionData.write("evolution_data.dat");
    system("gnuplot -persist plotscript.gp");
    return 0;
}