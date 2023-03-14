# clean the data files

import os
import json
import math
import numpy as np
os.chdir(os.path.dirname(__file__) + "/../../")


sets = {"A":"Set A", "B":"Set B", "CMT":"Christofides, Mingozzi and Toth", "E":"Set E","M":"Set M", "P":"Set P", "tai":"Rochat and Tailard", "X":"Uchoa et al.", "XXL":"Arnold, Genreau, Sorensen"}


def depot(nodes):

    # if the depot is at the end then place it at the begining
    if nodes[-1][3] == 0:

        # assure that the depot is not at the begining
        if nodes[0][3] != 0:

            # remove the depot from the end of the node list and add it to the begining
            depot = nodes.pop(-1)
            nodes.insert(0, depot)

    return nodes


# get the number of bits for each attribute
def encoding(nodes, data):
    bits = [0, 0, 0, 0]

    for i in range(4):

        # get the max value for each attribute
        maxValue = int(np.max(nodes, 0)[i])

        # iterate through to find the minimum number of bits needed to encode all the attributes
        for j in range(64):
            if maxValue < pow(2, j):
                bits[i] = j + 1
                break

    # record the encoding data
    data["X Bits"]       = bits[1]
    data["Y Bits"]       = bits[2]
    data["Demand Bits"]  = bits[3]

    # encode the nodes
    encodedNodes = []
    for n in nodes:
        encoded = (((int(n[1]) << bits[2]) + int(n[2]) << bits[3]) + int(n[3]))

        # check if the decoded value is equal to the actual value, otherwise exit out of the program
        x = (encoded >> (bits[2] + bits[3]))
        y = (encoded >> (bits[3])) % pow(2, bits[2])
        demand = encoded % pow(2, bits[3])

        if x != n[1] or y != n[2] or demand != n[3]:
            print("Encoded not equal to actual for problem", data["Problem"])
            exit()

        # else add the encoded value to the list
        encodedNodes.append(encoded)

    data["Encoded"] = encodedNodes
    return data


# get the node data
def nodes(start, lines, data):

    # get the node info
    nodes = []
    for n in range(data["Dimension"]):

        # get the node index, which equals the first line plus one to start at the first node, and the whatever node the current iteration is on
        nodeIndex = start + 1 + n

        # get the demand index, which equals the nodeIndex, plus the dimension of the nodes and one more
        demandIndex = nodeIndex + data["Dimension"] + 1

        # get the actual node and demand
        node = lines[nodeIndex].split()
        demand = float(lines[demandIndex].split()[-1])

        # get all the node information together
        nodes.append([n, float(node[1]), float(node[2]), demand])

    # get the encoding data of the nodes
    data = encoding(nodes, data)

    # get the depot info
    nodes = depot(nodes)

    # set the nodes
    data["Nodes"] = [[int(attr) for attr in node] for node in nodes]

    return data


# read in the data
def read(dir, file):
    data = {"Problem":file[:-4], "Set":sets[dir], "Capacity":None, "Dimension":None, "Best":None, "Optimal":None, "X Bits":None, "Y Bits":None, "Demand Bits":None, "Nodes":None, "Encoded":None}

    # read in the data from the raw data file
    with open("data/raw/" + dir + "/" + file, "r") as f:
        lines = f.readlines()
        for index, line in enumerate(lines):

            # get the best known value
            if line[0:3] == "COM":
                if len(line.split("value:")) == 2:
                    data["Best"] = float(line.split("value:")[1][:-2])
                elif len(line.split()) == 3:
                    data["Best"] = float(line.split()[-1])

            # determine if the best known is optimal
            elif line[0:3] == "OPT":
                if line.split(" : ")[1][0] == "T":  
                    data["Optimal"] = True
                else:
                    data["Optimal"] = False

            # determine the number of nodes
            elif line[0:3] == "DIM":
                data["Dimension"] = int(line.split(" : ")[-1])

            # determine the vehicle capacity
            elif line[0:3] == "CAP":
                data["Capacity"]  = int(line.split(" : ")[-1])

            # get the node information
            elif line[0:3] == "NOD":
                return nodes(index, lines, data)


def main(dir, file):
    # get the data from the file
    data = read(dir, file)

    # determine if any data is missing
    for i in data:
        if data[i] == None:
            if i != "Nodes":
                print("Missing values", file, i)
                exit()

    # get the name for the problem instance
    if   dir == "tai"   : newName = "tai-"    + file[3:-4]
    elif dir == "CMT"   : newName = "CMT-"    + file[3:-4]
    elif dir == "Li"    : newName = "Li-"     + file[3:-4]
    elif dir == "Golden": newName = "Golden-" + file[7:-4]
    elif dir == "XXL"   : newName = "XXL-"    + file[:-4]
    else:                 newName = file[:-4]

    # save the problem instance
    with open("data/test/" + dir + "/" + newName + ".json", "w") as f:
        json.dump(data, f, indent=4)

    # print the file name when finished
    print(file, "finished")


if __name__ == "__main__":
    for dir in os.listdir("data/raw/"):
        if dir not in os.listdir("data/test/"): os.mkdir("data/test/" + dir)
        for file in os.listdir("data/raw/" + dir):
            if file[-3:] == "vrp":
                if file[:-3] + "sol" in os.listdir("data/raw/" + dir):
                    main(dir, file)
