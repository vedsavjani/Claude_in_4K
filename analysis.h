#pragma once
#include <vector>
#include <complex>
#include <string>
#include "circuit.h"
#include "simulator.h"

// interface: an analysis runs a simulator on a circuit and reports results
class IAnalysis {
public:
    virtual ~IAnalysis() = default;
    virtual void run(const Circuit& ckt) = 0;     // compute and store results
    virtual void printResults() const = 0;        // display them
    virtual void exportCSV(const std::string& filename) const = 0;   // save results as a .csv file
};

// DC operating point: one solve at frequency 0
class DCAnalysis : public IAnalysis {
    ISimulator& sim;                       // which solver to use
    vector<complex<double>> result;        // node voltages
public:
    DCAnalysis(ISimulator& s) : sim(s) {}
    void run(const Circuit& ckt) override;
    void printResults() const override;
    void exportCSV(const std::string& filename) const override;
};

// AC sweep: one solve per frequency from startFreq to endFreq
class ACAnalysis : public IAnalysis {
    ISimulator& sim;
    double startFreq, endFreq, step;
    vector<double> freqs;                          // frequencies solved at
    vector<vector<complex<double>>> results;       // results[i] = node voltages at freqs[i]
public:
    ACAnalysis(ISimulator& s, double start, double end, double st);
    void run(const Circuit& ckt) override;
    void printResults() const override;
    void exportCSV(const std::string& filename) const override;
};