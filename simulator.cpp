#include "simulator.h"
#include <stdexcept>

vector<complex<double>> NodalSimulator::solve(const Circuit& circuit, double frequency) {
    const auto& comps = circuit.getComponents();

    // count voltage sources: each adds one extra unknown (its current)
    int numSources = 0;
    for (const auto& c : comps)
        if (c->getType() == "V") numSources++;

    int N = circuit.getNumNodes() - 1;   // unknown node voltages (ground excluded)
    int size = N + numSources;

    vector<vector<complex<double>>> A(size, vector<complex<double>>(size, 0.0));
    vector<complex<double>> b(size, 0.0);

    int srcIndex = N;   // source rows start after the node rows
    for (const auto& c : comps) {
        int a = c->getNodeA() - 1;    // node 1 -> row 0, ground -> -1
        int bn = c->getNodeB() - 1;

        if (c->getType() == "V") {
            // extra column: source current enters KCL at both nodes
            // extra row: Va - Vb = Vsource
            if (a >= 0)  { A[a][srcIndex] += 1.0;  A[srcIndex][a] += 1.0; }
            if (bn >= 0) { A[bn][srcIndex] -= 1.0; A[srcIndex][bn] -= 1.0; }
            b[srcIndex] = c->getValue();
            srcIndex++;
            continue;
        }

        // admittance y = 1/Z, with DC special cases
        complex<double> y;
        if (frequency == 0 && c->getType() == "C") continue;   // open circuit: adds nothing
        if (frequency == 0 && c->getType() == "L") y = 1e9;    // short: huge admittance
        else y = 1.0 / c->getImpedance(frequency);

        if (a >= 0)            A[a][a]   += y;
        if (bn >= 0)           A[bn][bn] += y;
        if (a >= 0 && bn >= 0) { A[a][bn] -= y; A[bn][a] -= y; }
    }

    vector<complex<double>> x = gaussSolve(A, b);

    // node voltages: ground is 0, the rest come from the first N entries of x
    vector<complex<double>> V(circuit.getNumNodes(), 0.0);
    for (int i = 0; i < N; i++) V[i + 1] = x[i];
    return V;
}

// Gaussian elimination with partial pivoting: solves A x = b
vector<complex<double>> NodalSimulator::gaussSolve(vector<vector<complex<double>>> A, vector<complex<double>> b) {
    int n = b.size();

    for (int col = 0; col < n; col++) {
        // pick the row with the largest value in this column (numerical stability)
        int pivot = col;
        for (int r = col + 1; r < n; r++)
            if (abs(A[r][col]) > abs(A[pivot][col])) pivot = r;

        if (abs(A[pivot][col]) < 1e-15)
            throw runtime_error("singular matrix: check the circuit is connected to ground");

        swap(A[col], A[pivot]);
        swap(b[col], b[pivot]);

        // eliminate this column from all rows below
        for (int r = col + 1; r < n; r++) {
            complex<double> factor = A[r][col] / A[col][col];
            for (int k = col; k < n; k++) A[r][k] -= factor * A[col][k];
            b[r] -= factor * b[col];
        }
    }

    // back substitution, from the last row upward
    vector<complex<double>> x(n);
    for (int i = n - 1; i >= 0; i--) {
        complex<double> sum = b[i];
        for (int k = i + 1; k < n; k++) sum -= A[i][k] * x[k];
        x[i] = sum / A[i][i];
    }
    return x;
}