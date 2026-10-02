#include <iostream>
#include "components.h"
#include "circuit.h"
#include "simulator.h"

int passed = 0, failed = 0;

// compares a node voltage with the expected value, within a tolerance
void check(string test, complex<double> got, complex<double> expected, double tol = 1e-3) {
    if (abs(got - expected) < tol) { passed++; cout << "PASS  "; }
    else                           { failed++; cout << "FAIL  "; }
    cout << test << "  got=" << got << "  expected=" << expected << "\n";
}

// expects the solver to throw (broken circuit)
void expectError(string test, const Circuit& ckt, double f) {
    NodalSimulator sim;
    try {
        sim.solve(ckt, f);
        failed++; cout << "FAIL  " << test << "  (no error thrown)\n";
    } catch (const exception& e) {
        passed++; cout << "PASS  " << test << "  error: " << e.what() << "\n";
    }
}

int main() {
    NodalSimulator sim;

    // 1. DC voltage divider: 10V across two equal resistors -> 5V in the middle
    {
        Circuit c(3);
        c.addComponent(make_unique<VoltageSource>("V1", 1, 0, 10, 0));
        c.addComponent(make_unique<Resistor>("R1", 1, 2, 1000));
        c.addComponent(make_unique<Resistor>("R2", 2, 0, 1000));
        auto V = sim.solve(c, 0);
        check("divider V1", V[1], 10);
        check("divider V2", V[2], 5);
    }

    // 2. series RLC at 1 kHz (hand-calculated)
    {
        Circuit c(4);
        c.addComponent(make_unique<VoltageSource>("V1", 1, 0, 5, 0));
        c.addComponent(make_unique<Resistor>("R1", 1, 2, 100));
        c.addComponent(make_unique<Inductor>("L1", 2, 3, 10e-3));
        c.addComponent(make_unique<Capacitor>("C1", 3, 0, 1e-6));
        auto V = sim.solve(c, 1000);
        check("RLC 1kHz V2", V[2], {2.4064, -2.4982});
        check("RLC 1kHz V3", V[3], {3.9760, -4.1278});
    }

    // 3. same RLC at resonance: L and C cancel, all 5V drops across R
    {
        Circuit c(4);
        c.addComponent(make_unique<VoltageSource>("V1", 1, 0, 5, 0));
        c.addComponent(make_unique<Resistor>("R1", 1, 2, 100));
        c.addComponent(make_unique<Inductor>("L1", 2, 3, 10e-3));
        c.addComponent(make_unique<Capacitor>("C1", 3, 0, 1e-6));
        double f0 = 1.0 / (2 * PI * sqrt(10e-3 * 1e-6));
        auto V = sim.solve(c, f0);
        check("resonance V2", V[2], {0, 0});
        check("resonance V3", V[3], {0, -5});
    }

    // 4. DC with capacitor: no current through it, node sits near source voltage
    {
        Circuit c(3);
        c.addComponent(make_unique<VoltageSource>("V1", 1, 0, 10, 0));
        c.addComponent(make_unique<Resistor>("R1", 1, 2, 1000));
        c.addComponent(make_unique<Capacitor>("C1", 2, 0, 1e-6));
        c.addComponent(make_unique<Resistor>("R2", 2, 0, 1e6));  // without it, node 2 floats at DC
        auto V = sim.solve(c, 0);
        check("DC cap V2", V[2], 10.0 * 1e6 / (1e6 + 1000));
    }

    // 5. DC with inductor: inductor shorts node 2 to ground
    {
        Circuit c(3);
        c.addComponent(make_unique<VoltageSource>("V1", 1, 0, 10, 0));
        c.addComponent(make_unique<Resistor>("R1", 1, 2, 1000));
        c.addComponent(make_unique<Inductor>("L1", 2, 0, 10e-3));
        auto V = sim.solve(c, 0);
        check("DC inductor V2", V[2], 0);
    }

    // 6. two sources: 10V and 5V through equal resistors -> 7.5V in between
    {
        Circuit c(4);
        c.addComponent(make_unique<VoltageSource>("V1", 1, 0, 10, 0));
        c.addComponent(make_unique<VoltageSource>("V2", 3, 0, 5, 0));
        c.addComponent(make_unique<Resistor>("R1", 1, 2, 1000));
        c.addComponent(make_unique<Resistor>("R2", 2, 3, 1000));
        auto V = sim.solve(c, 0);
        check("two sources V2", V[2], 7.5);
    }

    // 7. reversed source polarity: + terminal on ground
    {
        Circuit c(2);
        c.addComponent(make_unique<VoltageSource>("V1", 0, 1, 5, 0));
        c.addComponent(make_unique<Resistor>("R1", 1, 0, 1000));
        auto V = sim.solve(c, 0);
        check("reversed source V1", V[1], -5);
    }

    // 8. balanced Wheatstone bridge: both middle nodes at 5V
    {
        Circuit c(4);
        c.addComponent(make_unique<VoltageSource>("V1", 1, 0, 10, 0));
        c.addComponent(make_unique<Resistor>("R1", 1, 2, 1000));
        c.addComponent(make_unique<Resistor>("R2", 2, 0, 1000));
        c.addComponent(make_unique<Resistor>("R3", 1, 3, 2000));
        c.addComponent(make_unique<Resistor>("R4", 3, 0, 2000));
        c.addComponent(make_unique<Resistor>("R5", 2, 3, 5000));
        auto V = sim.solve(c, 0);
        check("bridge V2", V[2], 5);
        check("bridge V3", V[3], 5);
    }

    // ---------- broken circuits: these must throw ----------

    // 9. floating node: node 3 exists but nothing touches it
    {
        Circuit c(4);
        c.addComponent(make_unique<VoltageSource>("V1", 1, 0, 10, 0));
        c.addComponent(make_unique<Resistor>("R1", 1, 2, 1000));
        c.addComponent(make_unique<Resistor>("R2", 2, 0, 1000));
        expectError("floating node", c, 0);
    }

    // 10. no path to ground
    {
        Circuit c(3);
        c.addComponent(make_unique<VoltageSource>("V1", 1, 2, 10, 0));
        c.addComponent(make_unique<Resistor>("R1", 1, 2, 1000));
        expectError("no ground", c, 0);
    }

    // 11. two different sources in parallel: impossible circuit
    {
        Circuit c(2);
        c.addComponent(make_unique<VoltageSource>("V1", 1, 0, 10, 0));
        c.addComponent(make_unique<VoltageSource>("V2", 1, 0, 5, 0));
        expectError("parallel sources", c, 0);
    }

    // 12. source shorted to itself (both terminals on same node)
    {
        Circuit c(2);
        c.addComponent(make_unique<VoltageSource>("V1", 1, 1, 5, 0));
        c.addComponent(make_unique<Resistor>("R1", 1, 0, 1000));
        expectError("self-shorted source", c, 0);
    }

    // 13. DC: capacitors cut node 2 off completely
    {
        Circuit c(3);
        c.addComponent(make_unique<VoltageSource>("V1", 1, 0, 10, 0));
        c.addComponent(make_unique<Capacitor>("C1", 1, 2, 1e-6));
        c.addComponent(make_unique<Capacitor>("C2", 2, 0, 1e-6));
        expectError("DC floating behind caps", c, 0);
    }

    cout << "\n" << passed << " passed, " << failed << " failed\n";
}