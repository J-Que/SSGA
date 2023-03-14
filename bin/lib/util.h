// util function

#include <atomic>
#include <string>
#include <random>
#include <fstream>
#include <algorithm>

// read in a problem's nodes
void read(int nodes[], int N) {

    // open the coordinates file
    std::ifstream file ("coordinates.csv");
    std::string line;
    int n = 0;
    // open file
    if (file.is_open()) {

        // go through each line and add the node to the list
        while (file) {
            getline (file, line);
            if (n < N) {
                nodes[n] = stoi(line.substr(0, std::string::npos));
            }
            n++;
        }
    }
}


// unpack the demand
int population_demand(int node, int demandBits) {
    return ((node >> 1) % (int) std::pow(2, demandBits));
}


// calculate the distance between two customers
int population_euclidean(int node1, int node2, int demandBits, int yBits) {
    int x1 = (node1 >> (1 + yBits + demandBits));
    int y1 = (node1 >> (1 + demandBits)) % (int) std::pow(2, yBits);
    int x2 = (node2 >> (1 + yBits + demandBits));
    int y2 = (node2 >> (1 + demandBits)) % (int) std::pow(2, yBits);
    int dx = x2 - x1;
    int dy = y2 - y1;
    return (int) std::sqrt(dx*dx + dy*dy);
}


void population_cost(int individual[], int nodeSize, int demandBits, int yBits, int depot, int capacity) {
    // set the cost of the individual to 0
    individual[0] = 0;

    // add a depot visit to the first node
    individual[1] = individual[1] << 1;
    individual[1] += 1;

    // get the load of the first node
    int load = population_demand(individual[1], demandBits);

    // iterate through the nodes
    for (int n = 2; n < nodeSize; n++) {

        // push the node 1 to the left for inserting depots
        individual[n] = individual[n] << 1;

        // get the demand of the current node
        int demand = population_demand(individual[n], demandBits);

        // if the current node violates the capacity, then the route is ended
        if (demand + load > capacity) {

            // the load is reset
            load = demand;

            // a depot is inserted
            individual[n] += 1;

            // the distance between the current node and the depot is added
            individual[0] += population_euclidean(individual[n], depot, demandBits, yBits);

            // the distance between the previous node and the depot is added
            individual[0] += population_euclidean(individual[n - 1], depot, demandBits, yBits);

        // else the next node does not violate the capacity and the route continues
        } else {

            // the demand is added to the load
            load += demand;

            // the distance between the last node and the current node is added
            individual[0] += population_euclidean(individual[n - 1], individual[n], demandBits, yBits);
        }
    }

    // the distance between the first node and the depot is added, as well as the last node and depot
    individual[0] += population_euclidean(individual[1], depot, demandBits, yBits);
    individual[0] += population_euclidean(individual[nodeSize - 1], depot, demandBits, yBits);
}


// initialize the population
template <size_t populationSize, size_t nodeSize>
void populate(int (&population)[populationSize][nodeSize], int nodes[nodeSize], std::atomic<int> &BEST, int demandBits, int yBits, int depot, int capacity) {

    // for shuffling
    std::random_device rd;
    std::mt19937 engine(rd());

    for (int m = 0; m < populationSize; m++) {

        // shuffle the nodes (except for the first one)
        std::shuffle(nodes + 1, nodes + nodeSize, engine);

        // add the shuffled nodes to the individual
        std::copy(nodes, nodes + nodeSize, population[m]);

        // get the cost of the population
        population_cost(population[m], nodeSize, demandBits, yBits, depot, capacity);

        if (population[m][0] < BEST) {BEST = population[m][0];}
    }
}
