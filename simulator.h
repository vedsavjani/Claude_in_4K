#pragma once
#include <vector>
#include <complex>
#include "circuit.h"

// interface: any simulator takes a circuit + frequency, returns node voltages
class ISimulator {
public:
    virtual ~ISimulator() = default;
    virtual vector<complex<double>> solve(const Circuit& circuit, double frequency) = 0;
};

// Modified Nodal Analysis (MNA) simulator
class NodalSimulator : public ISimulator {
public:
    vector<complex<double>> solve(const Circuit& circuit, double frequency) override;
private:
    vector<complex<double>> gaussSolve(vector<vector<complex<double>>> A, vector<complex<double>> b);
};