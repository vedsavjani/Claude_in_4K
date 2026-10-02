#include "analysis.h"
#include <iomanip>
#include <stdexcept>

// ---------- DCAnalysis ----------
void DCAnalysis::run(const Circuit& ckt) {
    result = sim.solve(ckt, 0);
}

void DCAnalysis::printResults() const {
    cout << "--- DC operating point ---\n";
    for (int i = 0; i < (int)result.size(); i++)
        cout << "V" << i << " = " << result[i].real() << " V\n";   // DC: imaginary part is 0
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