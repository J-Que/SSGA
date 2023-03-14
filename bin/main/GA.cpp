// original source C++ file
#include <chrono>
#include <random>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>
#include <algorithm>
#include "../lib/util.h"
#include "../lib/report.h"
#include <emmintrin.h>


std::atomic<int> BEST{10000000};
auto START = std::chrono::high_resolution_clock::now();


// function called on each thread 
inline void evolve(int population[_population_][_nodes_], int nodes[_nodes_], int tr) {             //@ _population_:_nodes_:_nodes_
    std::random_device rd;
    std::mt19937 engine(rd());
    std::uniform_int_distribution<> cutGenerator(1, _nodes_ - 1);                                   //@ _nodes_
    std::uniform_int_distribution<> randomParent(tr * _ipt_, (tr + 1) * _ipt_ - 1);                 //@ _ipt_:_ipt_
    std::uniform_int_distribution<> randIndividual(0, _population_ - 1);                            //@ _population_

    // conduct the crossovers
    for (int m = 0; m < _subGenerations_; m++) {                                                    //@ _subGenerations_

        // choose the parents
        int parent1 = randomParent(engine);
        int parent2 = randomParent(engine);

        // -------------------------------------------------------------------------------------
        // Crossover
        // -------------------------------------------------------------------------------------

        // get random cut ( and start of unaltered search)
        int crossoverCut = cutGenerator(engine);

        // intialize the offspring
        int offspring[_nodes_];                                                                     //@ _nodes_

        // conduct 1-point crossover
        std::copy(population[parent1], population[parent1] + crossoverCut, offspring);
        std::copy(population[parent2] + crossoverCut, population[parent2] + _nodes_, offspring + crossoverCut);     //@ _nodes_

        // -------------------------------------------------------------------------------------
        // Mutation
        // -------------------------------------------------------------------------------------

        // generate a random number and determine if mutation will occur
        if (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) <= _mutationRate_) {          //@ _mutationRate_

            // get random cuts
            int mutationCut1 = cutGenerator(engine), mutationCut2 = cutGenerator(engine);

            // swap the cuts if cut1 is larger
            if (mutationCut1 > mutationCut2) {std::swap(mutationCut1, mutationCut2);}

            // perform inverse mutation
            std::reverse(offspring + mutationCut1, offspring + mutationCut2);
        }

        // -------------------------------------------------------------------------------------
        // Adjustment
        // -------------------------------------------------------------------------------------

        // reset all the depot flags of the node
        int n, n_i, n_j, node_i = 1, node_j;
        for (n = 1; n < _nodes_; n++) {                                                             //@ _nodes_
            offspring[n] = (offspring[n] >> 1);
        }

        // replace duplicate nodes with missing nodes
        for (n_i = 1; n_i < _nodes_; n_i++) {                                                       //@ _nodes_

            // iterate through the nodes of the offspring
            for (n_j = 1; n_j < _nodes_; n_j++) {                                                   //@ _nodes_

                // is the node from the list present in the offspring?
                if (nodes[n_i] == offspring[n_j]) {
                    goto next;
                }
            }

            // if the node was not found, then the offspring is searched for duplicate nodes to replace
            for (; node_i < _nodes_ - 1; node_i++) {                                                //@ _nodes_
                for (node_j = node_i + 1; node_j < _nodes_; node_j++) {                             //@ _nodes_

                    // if the node is a duplicate
                    if (offspring[node_i] == offspring[node_j]) {

                        // replace the duplicate with the missing node and break out of the loop
                        offspring[node_j] = nodes[n_i];
                        goto next;
                    }
                }
            }

            // if the node was found over replaced, continue to next node
            next:;
        }

        // -------------------------------------------------------------------------------------
        // Routing
        // -------------------------------------------------------------------------------------

        // hold all the decoded nodes
        int decoded[_nodes_][4];                                                                    //@ _nodes_

        // mark the start of the route within the individual and the load of the route
        int start, load = 0;

        // hold all the beginings (and ends) of the routes within the individual
        std::vector<int> routes {1};

        // decode all the nodes
        for (n = 1; n < _nodes_; n++) {                                                             //@ _nodes_

            // decode the node and add it to the decoded list (and push the original ID back by one to make room for the depot)
            decoded[n][0] = offspring[n] << 1;
            decoded[n][1] = offspring[n] >> (_yBits_ + _dBits_);                                    //@ _yBits_:_dBits_
            decoded[n][2] = (offspring[n] << (32 - _yBits_ - _dBits_)) >> ((32 - _yBits_ - _dBits_) + _dBits_);     //@ _yBits_:_dBits_:_yBits_:_dBits_:_dBits_
            decoded[n][3] = (offspring[n] - (((decoded[n][1] << _yBits_) + decoded[n][2]) << _dBits_));             //@ _yBits_:_dBits_

            // add the demand of the node to the load
            load += decoded[n][3];

            // if the capacity constraint has been violated, reset the start of the route, demand
            if (load > _capacity_) {                                                                //@ _capacity_
                start = n, load = decoded[n][3];
                routes.push_back(n);
            }
        }

        // mark the end of the last route
        routes.push_back(_nodes_);                                                                  //@ _nodes_

        // -------------------------------------------------------------------------------------
        // Fitness
        // -------------------------------------------------------------------------------------

        // reset the fitness for the individual
        offspring[0] = 0;
        float distance = 0;
        float distanceDelta = 0;

        // create the variables that will be used here
        int rSize, i, x, y;
        bool improved;
        int BDx, BDy, CEx, CEy, DEx, DEy, BCx, BCy;
        float BD, CE, DE, BC;

        // iterate through the routes
        for (int r = 1; r < routes.size(); r++) {

            // get the number of nodes in the route
            rSize = routes[r] - routes[r - 1] + 2;

            // identify the route
            int route[rSize][4];
            std::copy(&decoded[routes[r - 1]][0], &decoded[routes[r]][0], &route[1][0]);

            // add in the depots to the start and end of the route
            route[0][0] = _depot_;                                                                  //@ _depot_
            route[0][1] = _depotX_;                                                                 //@ _depotX_
            route[0][2] = _depotY_;                                                                 //@ _depotY_
            route[0][3] = 0;

            route[rSize - 1][0] = _depot_;                                                          //@ _depot_
            route[rSize - 1][1] = _depotX_;                                                         //@ _depotX_
            route[rSize - 1][2] = _depotY_;                                                         //@ _depotY_
            route[rSize - 1][3] = 0;

            // continue until no improvement has been made
            improved = true;
            while (improved) {
                improved = false;

                // iterate through the nodes of the routes
                for (n_i = 1; n_i < rSize - 2; n_i++) {

                    // iterate through the remaining potential nodes to swap
                    for (n_j = n_i + 1; n_j < rSize - 1; n_j++) {

                        // find the potential difference in distance
                        // A - B   C        A - B - C
                        //       x |   =>           |
                        // F - E   D        F - E - D
                        // A B D C E F    A B C D E F
                        // Example route: A → B → E → D → C → F → G → H → A
                        // Contents of new_route by step:
                        // A → B → E → D → C → F → G → H → A
                        // (A → B)
                        // A → B → (C → D → E)
                        // A → B → C → D → E → (F → G → H → A)
                        // A → D → C → B → A
                        BDx = route[n_i][1] - route[n_i - 1][1];
                        BDy = route[n_i][2] - route[n_i - 1][2];
                        BD = std::sqrt((BDx*BDx) + (BDy*BDy));
                        CEx = route[n_j][1] - route[n_j + 1][1];
                        CEy = route[n_j][2] - route[n_j + 1][2];
                        CE = std::sqrt((CEx*CEx) + (CEy*CEy));
                        DEx = route[n_i][1] - route[n_j + 1][1];
                        DEy = route[n_i][2] - route[n_j + 1][2];
                        DE = std::sqrt((DEx*DEx) + (DEy*DEy));
                        BCx = route[n_j][1] - route[n_i - 1][1];
                        BCy = route[n_j][2] - route[n_i - 1][2];
                        BC = std::sqrt((BCx*BCx) + (BCy*BCy));
                        distanceDelta = - BD + BC - CE + DE;

                        // If the length of the path is reduced, do a 2-opt swap
                        if (distanceDelta < 0) {
                            for (i = 0; i <= (n_j - n_i)/2; i++) {
                                std::swap_ranges(route[n_i + i], route[n_i + i] + 4, route[n_j - i]);
                            }
                            improved = true;
                        }
                    }
                }
            }

            // mark the begining of the route by inserting a depot visit to the node
            route[1][0]++;

            // copy the optimized route to the offspring
            for (n = 1; n < rSize - 1; n++) {
                offspring[routes[r - 1] + n - 1] = route[n][0];
            }

            // get the distance of the route
            for (n = 1; n < rSize; n++) {
                x = route[n][1] - route[n - 1][1];
                y = route[n][2] - route[n - 1][2];
                distance += std::sqrt(x*x + y*y);
            }
        }
        
        // set the fitness of the route
        offspring[0] = distance;

        // check if it is the new best fitness
        if (offspring[0] < BEST) {BEST = offspring[0];}

        // replace the worst parent in the population
        int (*worst)[_nodes_] = std::max_element(population + tr * _ipt_, population + (tr + 1) * _ipt_, [](const int a[_nodes_], const int b[_nodes_]){return a[0] < b[0];});       //@ _nodes_:_ipt_:_ipt_:_nodes_:_nodes_
        std::copy(offspring, offspring + _nodes_, (*worst));                                        //@ _nodes_
    }
}


int main(int argc, char *argv[]) {
    // save the progression of the solution
    std::vector<std::vector<int>> progression;

    // vector to hold thread objects
    std::vector<std::thread> threads;

    // read in the nodes
    int nodes[_nodes_];                                                                             //@ _nodes_
    read(nodes, _nodes_);                                                                           //@ _nodes_

    // initialize populations based of the nodes
    int population[_population_][_nodes_];                                                          //@ _population_:_nodes_
    populate(population, nodes, BEST, _dBits_, _yBits_, _depot_, _capacity_);                       //@ _dBits_:_yBits_:_depot_:_capacity_

    // print a header of the information and the results of the 0th generation
    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - START).count();
    header(BEST, _bestKnown_, time);                                                                //@ _bestKnown_

    // for randomization
    std::random_device rd;
    std::mt19937 engine(rd());
    std::uniform_int_distribution<> randIndividual(0, _population_ - 1);                            //@ _population_

    // iterate through the generations
    int generation = 1;
    for (; generation <= _generations_; generation++) {                                             //@ _generations_

        // shuffle the populaiton
        for (int m = 0; m < _population_; m++) {                                                    //@ _population_
            int randIndex = randIndividual(engine);
            std::swap_ranges(population[m], population[m] + _nodes_, population[randIndex]);        //@ _nodes_
        }

        // evolve the individuals in parrallel (launch the threads)
        for (int tr = 0; tr < _threads_; tr++) {                                                    //@ _threads_
            threads.emplace_back(evolve, population, nodes, tr);
        }

        // join the threads
        for (int tr = 0; tr < _threads_; tr++) {                                                    //@ _threads_
            threads[tr].join();
        }
        threads.clear();

        // if the cvrp has a known optimal solution, then the problem is terminated early
        if (_optimal_ && BEST + 1 <= _bestKnown_) {break;}                                          //@ _optimal_:_bestKnown_

        // update the results
        if (generation % 100 == 0) {
            int (*best)[_nodes_] = std::min_element(population, population + _population_, [](const int a[_nodes_], const int b[_nodes_]){return a[0] < b[0];});    //@ _nodes_:_population_:_nodes_:_nodes_
            progression.push_back(std::vector<int>(std::begin((*best)), std::end((*best))));
        }
        if (generation % _print_ == 0) {                                                            //@ _print_
            auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - START).count();
            update(generation, BEST, _bestKnown_, time);                                            //@ _bestKnown_
        }
    }

    // save the results
    time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - START).count();
    int (*best)[_nodes_] = std::min_element(population, population + _population_, [](const int a[_nodes_], const int b[_nodes_]){return a[0] < b[0];});    //@ _nodes_:_population_:_nodes_:_nodes_
    save(progression, (*best), BEST, generation, time);

    std::cout << "\n------------------------------\nTOTAL TIME:\n" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - START).count() << "\n";
    return 0;
}
