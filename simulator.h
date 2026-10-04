#pragma once

#include <vector>
#include <complex>
#include <string>
#include <map>
#include <chrono>
#include <iostream>
#include "circuit.h"

// ============================================================================
// OOP Concept 1: Pure Abstract Interface
// Defines the contract that every circuit simulator must fulfill.
// ============================================================================
class ISimulator {
public:
    virtual ~ISimulator() = default;

    // Pure virtual method: must be overridden by any concrete simulator
    virtual std::vector<std::complex<double>> solve(const Circuit& circuit, double frequency) = 0;
};

// ============================================================================
// OOP Concept 2: Template Method Pattern & Protected Helpers (Base Class)
// Implements the common solve pipeline while providing hook methods for children.
// ============================================================================
class BaseSimulator : public ISimulator {
protected:
    // Core MNA matrix assembler: stamps components into system matrix A and vector b
    virtual void buildSystem(const Circuit& circuit, double frequency,
                             std::vector<std::vector<std::complex<double>>>& A,
                             std::vector<std::complex<double>>& b);

    // Direct linear equation solver using Gaussian elimination with partial pivoting
    virtual std::vector<std::complex<double>> gaussSolve(
        std::vector<std::vector<std::complex<double>>> A,
        std::vector<std::complex<double>> b);

    // Virtual hooks: derived classes override these to inject custom behaviors
    virtual void preSolveHook(const Circuit& circuit, double frequency);
    virtual void postSolveHook(const Circuit& circuit, double frequency,
                               const std::vector<std::complex<double>>& nodeVoltages,
                               const std::vector<std::complex<double>>& fullSolution);

public:
    virtual ~BaseSimulator() = default;

    // Template method: defines the invariant simulation workflow skeleton
    std::vector<std::complex<double>> solve(const Circuit& circuit, double frequency) override;
};

// ============================================================================
// OOP Concept 3: Concrete Implementation (Default MNA Solver)
// Maintains exact backwards-compatibility with tests.cpp and analysis.h.
// ============================================================================
class NodalSimulator : public BaseSimulator {
public:
    NodalSimulator() = default;
};

// ============================================================================
// OOP Concept 4: Behavioral Extension & Profiling via Hook Methods
// Overrides pre/post hooks to monitor matrix size, solve count, and runtime.
// ============================================================================
class DiagnosticSimulator : public NodalSimulator {
private:
    int solveCount = 0;
    double lastDurationMs = 0.0;
    std::chrono::high_resolution_clock::time_point startTime;

protected:
    void preSolveHook(const Circuit& circuit, double frequency) override;
    void postSolveHook(const Circuit& circuit, double frequency,
                       const std::vector<std::complex<double>>& nodeVoltages,
                       const std::vector<std::complex<double>>& fullSolution) override;

public:
    DiagnosticSimulator() = default;
    int getSolveCount() const { return solveCount; }
    double getLastDurationMs() const { return lastDurationMs; }
};

// ============================================================================
// OOP Concept 5: Domain Specialization & Invariant Precondition Enforcement
// Restricts operation strictly to DC (f = 0 Hz), rejecting AC frequencies.
// ============================================================================
class DCSimulator : public NodalSimulator {
protected:
    // Enforces the precondition that frequency must be zero
    void preSolveHook(const Circuit& circuit, double frequency) override;

public:
    DCSimulator() = default;
    using BaseSimulator::solve;
    // Convenience overload specifically for DC operating point analysis
    std::vector<std::complex<double>> solve(const Circuit& circuit);
};

// ============================================================================
// OOP Concept 6: State Extension & Post-Processing Polymorphism
// Augments the simulator to calculate and store branch currents and power.
// ============================================================================
class CurrentTrackingSimulator : public NodalSimulator {
private:
    std::map<std::string, std::complex<double>> branchCurrents;
    std::map<std::string, double> powerDissipation;

protected:
    // Calculates currents and power from node potentials and MNA auxiliary rows
    void postSolveHook(const Circuit& circuit, double frequency,
                       const std::vector<std::complex<double>>& nodeVoltages,
                       const std::vector<std::complex<double>>& fullSolution) override;

public:
    CurrentTrackingSimulator() = default;

    std::complex<double> getBranchCurrent(const std::string& name) const;
    double getPower(const std::string& name) const;
    void printSummary() const;
};  
