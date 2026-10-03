#include "components.h"
#include "circuit.h"
#include "simulator.h"
#include "analysis.h"

int main() {
    // series RLC: V1 -> R1 -> L1 -> C1 -> ground
    Circuit ckt(4);
    ckt.addComponent(make_unique<VoltageSource>("V1", 1, 0, 5, 0));
    ckt.addComponent(make_unique<Resistor>("R1", 1, 2, 100));
    ckt.addComponent(make_unique<Inductor>("L1", 2, 3, 10e-3));
    ckt.addComponent(make_unique<Capacitor>("C1", 3, 0, 1e-6));
    ckt.listComponents();

    NodalSimulator sim;

    DCAnalysis dc(sim);
    dc.run(ckt);
    dc.printResults();

    ACAnalysis ac(sim, 500, 3000, 250);   // resonance is ~1592 Hz
    ac.run(ckt);
    ac.printResults();

    // save the results as CSV files (open them in Excel, or load with pandas)
    try {
        dc.exportCSV("dc_results.csv");
        ac.exportCSV("ac_sweep.csv");
        cout << "\nExported dc_results.csv and ac_sweep.csv\n";
    } catch (const exception& e) {
        cerr << "Export failed: " << e.what() << "\n";
        return 1;
    }
}