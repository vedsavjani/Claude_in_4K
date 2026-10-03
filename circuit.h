#pragma once
#include <vector>
#include <memory> // provides smart pointers
#include <iostream>
#include "components.h"

class Circuit {
    vector <unique_ptr<Component>> components;
    int numNodes;     // number of nodes
    int groundNode;   // keep it node 0
public:
    Circuit(int nodes) : numNodes(nodes), groundNode(0) {}

    void addComponent(unique_ptr<Component> comp) {
        components.push_back(move(comp));
    }

    void listComponents() const {
        for (const auto& c : components) {
            cout <<c->getId()<<" "<<c->getName() << "  " << c->getType() << "  value=" << c->getValue()
                 << "  nodes=" << c->getNodeA() << "," << c->getNodeB() << endl;
        }
    }

    // some getter functions
    int getNumNodes() const { 
        return numNodes; 
    }

    int getGroundNode() const { 
        return groundNode; 
    }
    const vector<unique_ptr<Component>>& getComponents() const { return components; }
};