#include <iostream>
#include <vector>
#include "components.h"
#include "circuit.h"

int main() {
    Circuit ckt(4);
    ckt.addComponent(make_unique<Resistor>("R1", 1, 2, 100));
    ckt.addComponent(make_unique<Inductor>("L1", 2, 3, 10e-3));
    ckt.addComponent(make_unique<Capacitor>("C1", 3, 0, 1e-6));
    ckt.addComponent(make_unique<VoltageSource>("V1", 1, 0, 5, 1000));

    ckt.listComponents();

    double f = 1000;
    for (const auto& c : ckt.getComponents()) {
        cout << c->getName() << "  Z=" << c->getImpedance(f) << "\n";
    }
}