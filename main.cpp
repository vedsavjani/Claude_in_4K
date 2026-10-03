#include <iostream>
#include "components.h"
#include "circuit.h"
#include "simulator.h"
#include "analysis.h"
#include "csv_io.h"

// the series RLC circuit used when no CSV file is given
Circuit buildDefaultCircuit() {
    // V1 -> R1 -> L1 -> C1 -> ground
    Circuit ckt(4);
    ckt.addComponent(make_unique<VoltageSource>("V1", 1, 0, 5, 0));
    ckt.addComponent(make_unique<Resistor>("R1", 1, 2, 100));
    ckt.addComponent(make_unique<Inductor>("L1", 2, 3, 10e-3));
    ckt.addComponent(make_unique<Capacitor>("C1", 3, 0, 1e-6));
    return ckt;
}

// usage:  minispice [circuit.csv]
int main(int argc, char* argv[]) {
    try {
        Circuit ckt = (argc > 1) ? readCircuitCsvFile(argv[1]) : buildDefaultCircuit();
        ckt.listComponents();

        NodalSimulator sim;

        DCAnalysis dc(sim);
        dc.run(ckt);
        dc.printResults();

        ACAnalysis ac(sim, 500, 3000, 250);   // resonance of the default circuit is ~1592 Hz
        ac.run(ckt);
        ac.printResults();

        // save the results (open them in Excel, or load them with pandas)
        writeResultsCsvFile("dc_results.csv", dc);
        writeResultsCsvFile("ac_results.csv", ac);
        cout << "\nSaved dc_results.csv and ac_results.csv\n";
    } catch (const CsvError& e) {
        cerr << "Circuit file problem: " << e.what() << "\n";
        return 1;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
