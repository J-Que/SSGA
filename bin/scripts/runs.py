import os
import sys
import json
import syn
import resource


# change the stack size to become unlimted and change the working directory
resource.setrlimit(resource.RLIMIT_STACK, (resource.RLIM_INFINITY, resource.RLIM_INFINITY))
os.chdir(os.path.dirname(__file__) + "/../../")


def main():  
    for rep in range(20):
        for pSet in os.listdir("data/test"):
            if pSet != "XXL":
                for problem in os.listdir("data/test/" + pSet):
            
                    # open the problem file
                    with open("data/test/" + problem.split("-")[0] + "/" + problem, "r") as f:
                        nodes = json.load(f)["Dimension"]

                    # get the parameters
                    with open("params.json", "r") as f:
                        params = json.load(f)

                    # update the parameters
                    params["problem"] = problem.split(".")[0]
                    params["ID"] = str(rep)
                    params["population size"] = 20 * nodes
            
                    # save the parameters
                    with open("params.json", "w") as f:
                        json.dump(params, f, indent=4)

                    # execute run
                    syn.main()


if __name__ == "__main__":
    main()
