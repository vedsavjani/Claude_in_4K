#pragma once
#include <vector>
#include <memory> // provides smart pointers
#include "components.h"

class Circuit {
    vector<unique_ptr<Component>> components;
    int numNodes;     // number of nodes
    int groundNode;   // keep it node 0
public:
    Circuit(int nodes);

    void addComponent(unique_ptr<Component> comp);
    void listComponents() const;

    // some getter functions
    int getNumNodes() const;
    int getGroundNode() const;
    const vector<unique_ptr<Component>>& getComponents() const;
};