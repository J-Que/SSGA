# synthesize the source C++ file

import os
import json
import subprocess
import validate
os.chdir(os.path.dirname(__file__) + "/../../")
CALL = "//@"


# open and get the parameters and attributes of the problem and algorithm
def read(presetParams=None):

    # determine if preset parameters are given
    if presetParams != None:
        data = presetParams

    # else read in the params
    else:
        with open("params.json", "r") as f:
            data = json.load(f)

    # reformat the parameters given for synthesizing
    params = {"_problem_": data["problem"], \
            "_generations_": data["generations"], \
            "_population_": data["population size"], \
            "_mutationRate_": data["mutation rate"], \
            "_threads_": data["threads"], \
            "_print_":data["printing interval"], \
            "_ID_": data["ID"], \
            '_ipt_': data["population size"] // data["threads"], \
            "_subGenerations_": data["sub-generations"]}

    # open the problem attributes file
    with open("data/test/" + params["_problem_"].split("-")[0] + "/" + params["_problem_"] + ".json", "r") as f:
        data = json.load(f)

    params.update({"_bestKnown_":data["Best"], \
                    "_optimal_": "true", \
                    "_nodes_":data["Dimension"], \
                    "_capacity_":data["Capacity"], \
                    "_depotX_":data["Nodes"][0][1], \
                    "_depotY_":data["Nodes"][0][2], \
                    "_depot_": data["Encoded"][0], \
                    "_dBits_":data["Demand Bits"], \
                    "_xBits_":data["X Bits"], \
                    "_yBits_":data["Y Bits"]})

    if data["Optimal"] == False: params["_optimal_"] = "false"
    
    return params, data["Encoded"]


# edit a line to write over it
def rewrite(tag, line, replacement):

    # get everything up to the tag
    edited = line[:line.find(tag)]

    # replace the tag
    edited += str(replacement)

    # add the remainder of the line
    edited += line[line.find(tag) + len(tag):]

    return edited


# choose which operators to use
def synthesize(params):

    # open the source and target files
    with open("bin/main/GA.cpp", "r") as source:
        with open("bin/main/src.cpp", "w") as target:

            # parse through the lines and find any where a call to edit the line is made
            for line in source:

                # get the line to be edited
                edited = line

                # find if a call to edit the line has been made
                if CALL in line.split():

                    # get all the tags for the call
                    tags = line[line.index(CALL) + 4:-1].split(":")

                    # re-write any value called to be rewritten
                    for tag in tags:
                        edited = rewrite(tag, edited, params[tag])
 
                # write the edited line (or unedited if no call was made) to the new file
                target.write(edited)


# create a coordinates file and write in the nodes
def coordinates(nodes): 
    with open("coordinates.csv", "w") as f:
        for node in nodes:
            f.write(str(node) + "\n")


# compile and execute the algorithm
def execute():
    subprocess.run(["g++", "-pthread", "-O3", "-march=native", "-o", "bin/main/GA.exe", "bin/main/src.cpp"])
    subprocess.run(["./bin/main/GA.exe"])


# conduct a single run of the GA
def main(presetParams=None):

    # read in the parameters
    params, nodes = read(presetParams)

    # synthesize the file with the parameters
    synthesize(params)

    # create a coordinate file for the algorithm
    coordinates(nodes)

    # compile and execute the program
    execute()


if __name__ == "__main__":

    # synthesize the file
    main(None)

    # get the problem
    file = os.listdir("results/reports")[0]
    problem = file.split("_")[0]

    # open the results
    with open("results/reports/" + file, "r") as f:
        results = json.load(f)

    # open the cvrp file
    with open("data/test/{}/{}.json".format(problem.split("-")[0], problem), "r") as f:
        attr = json.load(f)

    # validate the results
    nodes, cost, gap, valid = validate.main(results["genes"], attr)

    print("GA Cost: {}        Validated Cost: {}        Valid: {}".format(results["cost"], cost, valid))
