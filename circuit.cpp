#include "circuit.h"
#include <iostream>
#include <stdexcept>

Circuit::Circuit(int nodes) : numNodes(nodes), groundNode(0) {}

void Circuit::addComponent(unique_ptr<Component> comp) {
    // reject nodes outside 0..numNodes-1, otherwise the simulator writes outside the matrix
    int a = comp->getNodeA(), b = comp->getNodeB();
    if (a < 0 || a >= numNodes || b < 0 || b >= numNodes)
        throw invalid_argument(comp->getName() + " uses a node outside 0.." + to_string(numNodes - 1));
    components.push_back(move(comp));
}

void Circuit::listComponents() const {
    for (const auto& c : components) {
        cout << c->getName() << "  " << c->getType() << "  value=" << c->getValue()
             << "  nodes=" << c->getNodeA() << "," << c->getNodeB() << endl;
    }
}

int Circuit::getNumNodes() const { return numNodes; }
int Circuit::getGroundNode() const { return groundNode; }
const vector<unique_ptr<Component>>& Circuit::getComponents() const { return components; }