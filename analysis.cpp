#include "analysis.h"
#include <fstream>
#include <iomanip>
#include <stdexcept>

// ---------- CSV helpers (shared by both analyses) ----------
// opens the file for writing, or throws if that is not possible
static ofstream openCSV(const string& filename) {
    ofstream out(filename);
    if (!out) throw runtime_error("cannot open '" + filename + "' for writing");
    out << setprecision(10);          // plenty of digits for analysis, no ugly noise
    return out;
}

// makes sure everything was really written (disk full, etc.)
static void finishCSV(ofstream& out, const string& filename) {
    out.flush();
    if (!out) throw runtime_error("error while writing '" + filename + "'");
}

// ---------- DCAnalysis ----------
void DCAnalysis::run(const Circuit& ckt) {
    result = sim.solve(ckt, 0);
}

void DCAnalysis::printResults() const {
    cout << "--- DC operating point ---\n";
    for (int i = 0; i < (int)result.size(); i++)
        cout << "V" << i << " = " << result[i].real() << " V\n";   // DC: imaginary part is 0
}

// one row per node: node,voltage_V
void DCAnalysis::exportCSV(const string& filename) const {
    if (result.empty()) throw logic_error("DCAnalysis: call run() before exportCSV()");
    ofstream out = openCSV(filename);
    out << "node,voltage_V\n";
    for (int i = 0; i < (int)result.size(); i++)
        out << i << "," << result[i].real() << "\n";      // DC: imaginary part is 0
    finishCSV(out, filename);
}

// ---------- ACAnalysis ----------
ACAnalysis::ACAnalysis(ISimulator& s, double start, double end, double st)
    : sim(s), startFreq(start), endFreq(end), step(st) {
    if (start <= 0 || end < start || st <= 0)
        throw invalid_argument("AC sweep needs 0 < start <= end and step > 0");
}

void ACAnalysis::run(const Circuit& ckt) {
    freqs.clear();
    results.clear();
    int points = (int)((endFreq - startFreq) / step) + 1;
    for (int i = 0; i < points; i++) {
        double f = startFreq + i * step;    // computed from i, so no rounding drift
        freqs.push_back(f);
        results.push_back(sim.solve(ckt, f));
    }
}

void ACAnalysis::printResults() const {
    cout << "--- AC sweep: |V| (V) and phase (deg) per node ---\n";
    cout << fixed << setprecision(3);
    for (int i = 0; i < (int)freqs.size(); i++) {
        cout << setw(10) << freqs[i] << " Hz";
        for (int n = 1; n < (int)results[i].size(); n++)          // skip ground
            cout << "  |  V" << n << " " << abs(results[i][n])
                 << " @ " << arg(results[i][n]) * 180 / PI;
        cout << "\n";
    }
    cout.unsetf(ios::fixed);
}

// one row per frequency: frequency_Hz, then |V| and phase for every non-ground node
void ACAnalysis::exportCSV(const string& filename) const {
    if (freqs.empty()) throw logic_error("ACAnalysis: call run() before exportCSV()");
    ofstream out = openCSV(filename);
    int nodes = (int)results[0].size();

    out << "frequency_Hz";
    for (int n = 1; n < nodes; n++)                           // skip ground
        out << ",V" << n << "_mag_V,V" << n << "_phase_deg";
    out << "\n";

    for (int i = 0; i < (int)freqs.size(); i++) {
        out << freqs[i];
        for (int n = 1; n < nodes; n++)
            out << "," << abs(results[i][n]) << "," << arg(results[i][n]) * 180 / PI;
        out << "\n";
    }
    finishCSV(out, filename);
}
