#include <iostream>
#include <vector>
#include "components.h"

int main() {
    vector<Component*> parts;
    parts.push_back(new Resistor("R1", 1, 2, 100));
    parts.push_back(new Inductor("L1", 2, 3, 10e-3));
    parts.push_back(new Capacitor("C1", 3, 0, 1e-6));
    parts.push_back(new VoltageSource("V1", 1, 0, 5, 1000));

    double f = 1000;
    for (Component* p : parts) {
        cout << p->getName() << "  type=" << p->getType()
             << "  value=" << p->getValue()
             << "  Z=" << p->getImpedance(f)
             << "  nodes=" << p->getNodeA() << "," << p->getNodeB() << "\n";
    }

    for (Component* p : parts) delete p;
}