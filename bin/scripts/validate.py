# validate and compile results

import os
import math
import json
os.chdir(os.path.dirname(__file__) + "/../../")


# unpack all the genes
def decode(genes, cvrp):

    # identify the information needed from the original problem file
    depotGene = cvrp["Nodes"][0]
    yBits = cvrp["Y Bits"]
    demandBits = cvrp["Demand Bits"]

    # create a nodes list
    decoded = []

    # iterate through the genes
    for gene in genes:

        # is there a depot visit before the node, then add a depot visit
        if gene % 2 == 1:
            decoded.append(depotGene)
            gene -= 1

        # unpack the gene
        x = (gene >> (1 + yBits + demandBits))
        y = (gene >> (1 + demandBits)) % pow(2, yBits)
        demand = (gene >> 1) % pow(2, demandBits)

        # append the unpack gene to the list
        decoded.append([0, x, y, demand])

    # add a depot at the end
    decoded.append(depotGene)

    return decoded


# determine if the unpacking the bits gives the original node information
def lookup(nodes, cvrp):
    valid = True

    # iterate through the nodes in the orignal node list
    for ni, nodei in enumerate(cvrp["Nodes"]):
        found = False

        # iterate through the nodes in the run
        for nj, nodej in enumerate(nodes):

            # determine if all the nodes from the orignal list are present and correct in the solution node list
            if nodei[1] == nodej[1]:
                if nodei[2] == nodej[2]:
                    if nodei[3] == nodej[3]:

                        # get the index value
                        nodes[nj][0] = nodei[0]
                        found = True
                        break

        if not found:
            print("Missing Node! {}".format(cvrp["Problem"]))
            valid = False


    return nodes, valid


# get the euclidean distance between two nodes
def euclidean(node1, node2):
    dx = node1[1] - node2[1]
    dy = node1[2] - node2[2]
    x2 = dx*dx
    y2 = dy*dy
    return math.sqrt(x2+y2)


# determine if the capacity constraint has been violated
def capacity(nodes, cvrp):

    # get the indomration needed from the orignal problem file
    truckCapacity = cvrp["Capacity"]
    load = 0
    distance = 0

    # iterate through the nodes
    for n, node in enumerate(nodes):
           
        # skip over the first node
        if n == 0: continue

        demand = node[3]
        distance += euclidean(nodes[n], nodes[n-1])

        # determine if the node is a depot
        if demand == 0:

            # if it is, then the load is rest
            load = 0

        # if it is not, then determine if the capacity constrain has been violated
        elif load + demand <= truckCapacity:

            # if it does not, then the demand is added to the load
            load += demand

        # else it does violated the capacity constraint
        else:
            print("Capacity constraint violated! capacity {} and load {} {}".format(capacity, load, cvrp["Problem"]))
            return 0, 0, False

    return distance, (distance - cvrp["Best"])/cvrp["Best"], True


# validate results
def main(genes, cvrp):

    # decode the genes
    nodes = decode(genes, cvrp)

    # determine if all the nodes are present and correct
    nodes, noMissingNodes = lookup(nodes, cvrp)

    # determine if the capacity constraint has been violated
    distance, gap, validCapacity = capacity(nodes, cvrp)

    return nodes, distance, gap, validCapacity and noMissingNodes


