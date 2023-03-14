// used for saving and reporting results
#include <ctime>
#include <fstream>
#include <string>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <vector>

// read in the parameter file
void params(std::string &cvrp, std::string &ID, std::string &population, std::string &generations, std::string &threads, std::string &mutationRate, std::string &subGenerations) {
    std::ifstream params ("params.json");
    std::string line, delimiter = ": ", lineEnd = ",";

    // read through the file
    if (params.is_open()) {
        int lineCount = 0;

        // go through each line
        while (params) {
            std::getline(params, line);

            int start = line.find(delimiter) + 2;
            int end = line.find(lineEnd) - start;

            if (lineCount == 1) {ID = line.substr(1 + start, end - 2);}
            else if (lineCount == 2) {cvrp = line.substr(1 + start, end - 2);}
            else if (lineCount == 3) {generations = line.substr(start, end);}
            else if (lineCount == 4) {population = line.substr(start, end);}
            else if (lineCount == 5) {mutationRate = line.substr(start, end);}
            else if (lineCount == 6) {threads = line.substr(start, end);}
            else if (lineCount == 7) {subGenerations = line.substr(start, end);}
            lineCount++;
        }
    }
}


// read in the attribute file for the problem
void problem(std::string cvrp, std::string &nodes, std::string &best) {
    std::string dir = cvrp.substr(0, cvrp.find("-"));
    std::string path = "data/test/" + dir + "/" + cvrp + ".json";

    std::ifstream problem(path);
    std::string line, delimiter = ": ", lineEnd = ",";

    // read through the file
    if (problem.is_open()) {
        int lineCount = 0;

        // go through each line
        while (problem) {
            std::getline(problem, line);

            int start = line.find(delimiter) + 2;
            int end = line.find(lineEnd) - start;

            if (lineCount == 4) {nodes = line.substr(start, end);}
            else if (lineCount == 5) {best = line.substr(start, end);}
            lineCount++;
        }
    }
}


// update after every few generations
void update(int generation, int cost, float optimal, float t) {
    std::cout << "   " << std::setw(8) << generation << "    " << std::setw(12) << cost << "     " << std::setw(12) << (cost - optimal)/optimal*100 << "        " << std::setw(12) << t/1000 << "            " << std::setw(8) << (t/1000)/generation << std::endl;
}


// print a summary of the problem
void header(int cost, float optimal, float t) {
    std::string subGenerations, mutationRate, population, generations, threads;
    std::string cvrp, ID, nodes, best;
    params(cvrp, ID, population, generations, threads, mutationRate, subGenerations);
    problem(cvrp, nodes, best);

    // format the output of the results
    std::cout << std::left;

    std::cout << std::endl;
    for (int i = 0; i < 110; i++) {std::cout << "_";}

    std::cout << "\n\n  " << "Genetic Algorithm with Two-Opt Optimization" << std::endl;
    for (int i = 0; i < 110; i++) {std::cout << "-";}
    std::cout << "\n  CVRP: " << std::setw(20) << cvrp << "  Nodes: " << std::setw(19) << nodes << "  Best Known: " << std::setw(14) << best;
    std::cout << "\n  Generations: " << std::setw(13) << generations << "  Population Size: " << std::setw(9) << population << "  Threads: " << std::setw(17) << threads << std::endl;

    for (int i = 0; i < 110; i++) {std::cout << "-";}
    std::cout << "\n  Gen          Cost             Gap %               Time (sec)              Second per Generation" << std::endl;
    for (int i = 0; i < 110; i++) {std::cout << "-";}
    std::cout << std::endl;

    // print the results of the 0th generation
    update(0, cost, optimal, t);
}


// save the results
void save(std::vector<std::vector<int>> progression, int solution[], int cost, int generationReached, float t) {
    std::string subGenerations, mutationRate, population, generations, threads;
    std::string cvrp, ID, nodes, best;
    params(cvrp, ID, population, generations, threads, mutationRate, subGenerations);
    problem(cvrp, nodes, best);
    std::ofstream results ("results/reports/" + cvrp + "_" + ID + ".json");

    // Get current time
    std::time_t tDate = std::time(0);
    // Convert current time to tm struct for local time
    std::tm* now = std::localtime(&tDate);
    // Extract the year, month, and day from the tm struct
    int year = now->tm_year + 1900;
    int month = now->tm_mon + 1;
    int day = now->tm_mday; 

    results << "{";
    results << "\n    \"problem\": \"" << cvrp << "\",";
    results << "\n    \"cost\": " << cost << ",";
    results << "\n    \"gap\": " << (cost - stof(best))/stof(best) << ",";
    results << "\n    \"date\": \"" << year << '-' << month << '-' << day << "\","; 
    results << "\n    \"population size\": " << population  << ",";
    results << "\n    \"threads\": " << threads << ",";
    results << "\n    \"generations\": " << generationReached << ",";
    results << "\n    \"time (milliseconds)\": " << t << ",";
    results << "\n    \"milliseconds per generation\": " << t / generationReached << ",";
    results << "\n    \"mutation rate\": " << mutationRate << ",";
    results << "\n    \"sub-generations\": " << subGenerations << ",";

    results << "\n    \"genes\": [";
    for (int n = 1; n < stoi(nodes) - 1; n++) {
        results << "\n        " << solution[n] << ",";
    }

    // add in the last depot
    results << "\n        " << solution[stoi(nodes) - 1];
    results << "\n    ],";    

    // save the progression of the run
    results << "\n    \"progression\": {";
    for (int gen = 0; gen < progression.size() - 1; gen++) {
        results << "\n        \"generation " << (gen + 1) * 100 << "\" : [\n            ";
        for (int n = 0; n < stoi(nodes) - 1; n++) {
            results << progression[gen][n] << ", ";
        }
        results << progression[gen][stoi(nodes) - 1] << "\n        ],";
    }
    results << "\n        \"generation " << (progression.size() + 1) * 100 << "\" : [\n            ";
    for (int n = 0; n < stoi(nodes) - 1; n++) {
        results << progression[progression.size() - 1][n] << ", ";
    }
    results << progression[progression.size() - 1][stoi(nodes) - 1] << "\n        ]";
    results << "\n    }";    
    results << "\n}";
    results.close();

    // print the last update
    update(generationReached, cost, stof(best), t);
}
