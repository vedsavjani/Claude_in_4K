#include "simulator.h"
#include <stdexcept>
#include <iomanip>
#include <cmath>

using namespace std;

// ============================================================================
// BaseSimulator Implementation (Template Method)
// ============================================================================

// Default empty hooks: derived classes may selectively override them
void BaseSimulator::preSolveHook(const Circuit&, double) {}
void BaseSimulator::postSolveHook(const Circuit&, double,
                                 const vector<complex<double>>&,
                                 const vector<complex<double>>&) {}

// Builds the MNA conductance/admittance matrix and independent source vector
void BaseSimulator::buildSystem(const Circuit& circuit, double frequency,
                                vector<vector<complex<double>>>& A,
                                vector<complex<double>>& b) {
    const auto& comps = circuit.getComponents();

    // Count independent voltage sources (each requires an auxiliary MNA row/column)
    int numSources = 0;
    for (const auto& c : comps) {
        if (c->getType() == "V") numSources++;
    }

    int N = circuit.getNumNodes() - 1; // Number of non-ground node unknowns
    int size = N + numSources;

    A.assign(size, vector<complex<double>>(size, 0.0));
    b.assign(size, 0.0);

    int srcIndex = N;
    for (const auto& c : comps) {
        int a = c->getNodeA() - 1;  // Node 1 -> Row 0, Ground (0) -> -1
        int bn = c->getNodeB() - 1;

        if (c->getType() == "V") {
            // Voltage source auxiliary equations: V_a - V_b = V_source
            if (a >= 0)  { A[a][srcIndex] += 1.0;  A[srcIndex][a] += 1.0; }
            if (bn >= 0) { A[bn][srcIndex] -= 1.0; A[srcIndex][bn] -= 1.0; }
            b[srcIndex] = c->getValue();
            srcIndex++;
            continue;
        }

        // Compute admittance y = 1 / Z with boundary handling for DC
        complex<double> y;
        if (frequency == 0.0 && c->getType() == "C") continue; // Capacitor = open circuit
        if (frequency == 0.0 && c->getType() == "L") y = 1e9;  // Inductor = ideal short circuit
        else y = 1.0 / c->getImpedance(frequency);

        // Standard 2x2 nodal admittance stamp
        if (a >= 0)            A[a][a]   += y;
        if (bn >= 0)           A[bn][bn] += y;
        if (a >= 0 && bn >= 0) { A[a][bn] -= y; A[bn][a] -= y; }
    }
}

// Solves Ax = b using Gaussian elimination with partial pivoting
vector<complex<double>> BaseSimulator::gaussSolve(vector<vector<complex<double>>> A,
                                                  vector<complex<double>> b) {
    int n = static_cast<int>(b.size());

    for (int col = 0; col < n; col++) {
        // Select pivot row with largest magnitude for numerical stability
        int pivot = col;
        for (int r = col + 1; r < n; r++) {
            if (abs(A[r][col]) > abs(A[pivot][col])) pivot = r;
        }

        // Matrix is singular if pivot is zero (floating node, missing ground, shorted source)
        if (abs(A[pivot][col]) < 1e-15)
            throw runtime_error("singular matrix: check the circuit is connected to ground");

        swap(A[col], A[pivot]);
        swap(b[col], b[pivot]);

        // Eliminate column entries below the pivot
        for (int r = col + 1; r < n; r++) {
            complex<double> factor = A[r][col] / A[col][col];
            for (int k = col; k < n; k++) A[r][k] -= factor * A[col][k];
            b[r] -= factor * b[col];
        }
    }

    // Back substitution
    vector<complex<double>> x(n);
    for (int i = n - 1; i >= 0; i--) {
        complex<double> sum = b[i];
        for (int k = i + 1; k < n; k++) sum -= A[i][k] * x[k];
        x[i] = sum / A[i][i];
    }
    return x;
}

// Invariant template method orchestrating simulation steps
vector<complex<double>> BaseSimulator::solve(const Circuit& circuit, double frequency) {
    preSolveHook(circuit, frequency);

    vector<vector<complex<double>>> A;
    vector<complex<double>> b;
    buildSystem(circuit, frequency, A, b);

    vector<complex<double>> x = gaussSolve(A, b);

    // Node 0 is defined as Ground (0V); populate non-ground voltages
    int N = circuit.getNumNodes() - 1;
    vector<complex<double>> V(circuit.getNumNodes(), 0.0);
    for (int i = 0; i < N; i++) {
        V[i + 1] = x[i];
    }

    postSolveHook(circuit, frequency, V, x);
    return V;
}

// ============================================================================
// DiagnosticSimulator Implementation
// ============================================================================

void DiagnosticSimulator::preSolveHook(const Circuit& circuit, double frequency) {
    solveCount++;
    startTime = chrono::high_resolution_clock::now();
    cout << "[DIAGNOSTIC] Solve #" << solveCount << " started: "
         << circuit.getNumNodes() << " nodes, "
         << circuit.getComponents().size() << " components, f = "
         << frequency << " Hz\n";
}

void DiagnosticSimulator::postSolveHook(const Circuit&, double,
                                       const vector<complex<double>>&,
                                       const vector<complex<double>>&) {
    auto endTime = chrono::high_resolution_clock::now();
    lastDurationMs = chrono::duration<double, milli>(endTime - startTime).count();
    cout << "[DIAGNOSTIC] Solve completed in " << fixed << setprecision(3)
         << lastDurationMs << " ms\n";
}

// ============================================================================
// DCSimulator Implementation
// ============================================================================

void DCSimulator::preSolveHook(const Circuit&, double frequency) {
    if (frequency != 0.0) {
        throw invalid_argument("DCSimulator violation: frequency must be 0 Hz for DC analysis, got "
                               + to_string(frequency) + " Hz");
    }
}

vector<complex<double>> DCSimulator::solve(const Circuit& circuit) {
    return BaseSimulator::solve(circuit, 0.0);
}

// ============================================================================
// CurrentTrackingSimulator Implementation
// ============================================================================

void CurrentTrackingSimulator::postSolveHook(const Circuit& circuit, double frequency,
                                            const vector<complex<double>>& nodeVoltages,
                                            const vector<complex<double>>& fullSolution) {
    branchCurrents.clear();
    powerDissipation.clear();

    const auto& comps = circuit.getComponents();
    int N = circuit.getNumNodes() - 1;
    int srcIdx = 0;

    for (const auto& c : comps) {
        int a = c->getNodeA();
        int b = c->getNodeB();
        complex<double> vDrop = nodeVoltages[a] - nodeVoltages[b];
        complex<double> current = 0.0;

        if (c->getType() == "V") {
            // In MNA, branch current through voltage source is in auxiliary vector slot
            current = fullSolution[N + srcIdx];
            srcIdx++;
        } else if (c->getType() == "R") {
            current = vDrop / c->getValue();
        } else if (c->getType() == "C") {
            current = (frequency == 0.0) ? 0.0 : (vDrop / c->getImpedance(frequency));
        } else if (c->getType() == "L") {
            current = (frequency == 0.0) ? (vDrop * 1e9) : (vDrop / c->getImpedance(frequency));
        }

        branchCurrents[c->getName()] = current;
        // P = Re(V * I*)
        powerDissipation[c->getName()] = (vDrop * conj(current)).real();
    }
}

complex<double> CurrentTrackingSimulator::getBranchCurrent(const string& name) const {
    auto it = branchCurrents.find(name);
    if (it == branchCurrents.end()) throw invalid_argument("Component not found: " + name);
    return it->second;
}

double CurrentTrackingSimulator::getPower(const string& name) const {
    auto it = powerDissipation.find(name);
    if (it == powerDissipation.end()) throw invalid_argument("Component not found: " + name);
    return it->second;
}

void CurrentTrackingSimulator::printSummary() const {
    cout << "\n--- Branch Current & Power Summary ---\n";
    cout << left << setw(12) << "Component" << setw(20) << "Current (A)" << "Power (W)\n";
    cout << string(45, '-') << "\n";
    for (const auto& [name, i] : branchCurrents) {
        cout << left << setw(12) << name
             << setw(20) << abs(i)
             << powerDissipation.at(name) << "\n";
    }
}
