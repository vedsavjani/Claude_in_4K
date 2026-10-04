#include "csv_io.h"

#include <cctype>
#include <cmath>
#include <fstream>
#include <functional>
#include <locale>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <string>
#include <stdexcept>

using namespace std;

// ============================================================================
// CsvError
// ============================================================================
CsvError::CsvError(const string& source, size_t line, const string& message)
    : runtime_error(line ? source + ":" + to_string(line) + ": " + message
                         : source + ": " + message),
      source_(source), line_(line) {}

namespace {  // everything below is private to this file

const int kMaxNode = 1000;   // the solver uses a dense matrix, so huge node numbers are refused

// ============================================================================
// Reading: small text helpers
// ============================================================================
string trim(const string& s) {
    const size_t first = s.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    const size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

string lower(string s) { for (char& c : s) c = (char)tolower((unsigned char)c); return s; }
string upper(string s) { for (char& c : s) c = (char)toupper((unsigned char)c); return s; }

// Splits one CSV line into fields. Supports "quoted, fields" and "" for a literal quote.
vector<string> parseCsvRow(const string& line) {
    vector<string> fields;
    string field;
    bool inQuotes = false;     // currently inside "..."
    bool wasQuoted = false;    // this field started with a quote

    auto endField = [&]() {
        fields.push_back(wasQuoted ? field : trim(field));
        field.clear();
        wasQuoted = false;
    };

    for (size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (inQuotes) {
            if (ch != '"') field += ch;
            else if (i + 1 < line.size() && line[i + 1] == '"') { field += '"'; ++i; }
            else inQuotes = false;                                  // closing quote
        } else if (ch == ',') {
            endField();
        } else if (ch == '"') {
            if (wasQuoted || !trim(field).empty())
                throw invalid_argument("unexpected quote inside a field");
            field.clear();
            inQuotes = wasQuoted = true;                            // opening quote
        } else if (wasQuoted) {
            if (ch != ' ' && ch != '\t' && ch != '\r')
                throw invalid_argument("unexpected text after a quoted field");
        } else {
            field += ch;
        }
    }
    if (inQuotes) throw invalid_argument("unterminated quoted field");
    endField();
    return fields;
}

int parseNode(const string& s) {
    int node = 0;
    try {
        size_t pos = 0;
        node = stoi(s, &pos);
        if (pos != s.size() || node < 0) throw invalid_argument(""); 
    } catch (...) {
        throw invalid_argument("node must be a non-negative whole number, got '" + s + "'");
    }
    if (node > kMaxNode)
        throw invalid_argument("node number " + s + " is too large (maximum is " +
                               to_string(kMaxNode) + ")");
    return node;
}

double parseValueField(const string& s) {
    double number = 0.0;
    try {
        // plain numbers only: digits, sign, decimal point and e/E (no hex, inf, nan or unit suffixes)
        if (s.empty() || s.find_first_not_of("0123456789+-.eE") != string::npos)
            throw invalid_argument("");
        size_t pos = 0;
        number = stod(s, &pos);
        if (pos != s.size())                       // something left over, e.g. "5e" or "1.2.3"
            throw invalid_argument("");
    } catch (...) {
        throw invalid_argument("value '" + s + "' is not a valid number (use plain numbers such as 4700 or 4.7e3; suffixes like k or u are not supported)");
    }
    
    if (!isfinite(number))
        throw invalid_argument("value '" + s + "' is too large or not a valid number");
        
    return number;
}

// ============================================================================
// Reading: building components
// ============================================================================
double positive(double v, const char* what) {
    if (v <= 0) throw invalid_argument(string(what) + " must be greater than zero");
    return v;
}

// One entry per component type. To support a new component, add one line here.
using Maker = function<unique_ptr<Component>(const string&, int, int, double)>;

const map<string, Maker>& componentMakers() {
    static const map<string, Maker> makers = {
        {"R", [](const string& n, int a, int b, double v) -> unique_ptr<Component> {
            return make_unique<Resistor>(n, a, b, positive(v, "resistance")); }},
        {"L", [](const string& n, int a, int b, double v) -> unique_ptr<Component> {
            return make_unique<Inductor>(n, a, b, positive(v, "inductance")); }},
        {"C", [](const string& n, int a, int b, double v) -> unique_ptr<Component> {
            return make_unique<Capacitor>(n, a, b, positive(v, "capacitance")); }},
        {"V", [](const string& n, int a, int b, double v) -> unique_ptr<Component> {
            return make_unique<VoltageSource>(n, a, b, v, 0.0); }},
    };
    return makers;
}

void checkHeader(const vector<string>& fields) {
    static const vector<string> expected = {"type", "name", "node_a", "node_b", "value"};
    bool ok = fields.size() == expected.size();
    for (size_t i = 0; ok && i < expected.size(); ++i)
        ok = lower(fields[i]) == expected[i];
    if (!ok) throw invalid_argument("expected header: type,name,node_a,node_b,value");
}

unique_ptr<Component> parseComponentRow(const vector<string>& fields,
                                        set<string>& names, set<int>& usedNodes) {
    if (fields.size() != 5)
        throw invalid_argument("expected 5 fields: type,name,node_a,node_b,value");

    const string type = upper(fields[0]);
    const string& name = fields[1];
    if (name.empty()) throw invalid_argument("component name cannot be empty");
    if (names.count(name)) throw invalid_argument("duplicate component name: " + name);

    const int nodeA = parseNode(fields[2]);
    const int nodeB = parseNode(fields[3]);
    if (nodeA == nodeB)
        throw invalid_argument("both terminals of " + name + " are on node " + to_string(nodeA));
    const double value = parseValueField(fields[4]);

    auto maker = componentMakers().find(type);
    if (maker == componentMakers().end()) {
        string known;
        for (const auto& entry : componentMakers()) known += (known.empty() ? "" : ", ") + entry.first;
        throw invalid_argument("unsupported component type '" + fields[0] + "' (use " + known + ")");
    }
    auto component = maker->second(name, nodeA, nodeB, value);

    names.insert(name);              // only remember them once the row is fully valid
    usedNodes.insert(nodeA);
    usedNodes.insert(nodeB);
    return component;
}

// ============================================================================
// Writing
// ============================================================================

// Temporarily gives a stream CSV-friendly formatting (15 digits, plain numbers,
// '.' as decimal point) and puts the caller's settings back afterwards (RAII).
class FormatGuard {
    ostream& os_;
    ios::fmtflags flags_;
    streamsize precision_;
    locale locale_;
public:
    explicit FormatGuard(ostream& os)
        : os_(os), flags_(os.flags()), precision_(os.precision()),
          locale_(os.imbue(locale::classic())) {
        os_.unsetf(ios::floatfield);
        os_.precision(15);
    }
    ~FormatGuard() {
        os_.imbue(locale_);
        os_.flags(flags_);
        os_.precision(precision_);
    }
    FormatGuard(const FormatGuard&) = delete;
    FormatGuard& operator=(const FormatGuard&) = delete;
};

void writeHeader(ostream& os) {
    os << "frequency_hz,node,voltage_real_v,voltage_imag_v,magnitude_v,phase_deg\n";
}

void writeRow(ostream& os, double frequency, size_t node, complex<double> v) {
    v = {v.real() + 0.0, v.imag() + 0.0};      // "+ 0.0" turns a negative zero into 0
    os << frequency << ',' << node << ',' << v.real() << ',' << v.imag() << ','
       << abs(v) << ',' << arg(v) * 180.0 / PI << '\n';
}

// throws before anything is written if run() was never called
void requireResults(const DCAnalysis& analysis) {
    if (analysis.getResult().empty())
        throw logic_error("run the DC analysis before exporting its results");
}
void requireResults(const ACAnalysis& analysis) {
    if (analysis.getFrequencies().empty() || analysis.getResults().size() != analysis.getFrequencies().size())
        throw logic_error("run the AC analysis before exporting its results");
}

template <typename Analysis>
void writeToFile(const string& path, const Analysis& analysis) {
    requireResults(analysis);              // first! opening the file would erase an existing one
    ofstream out(path);
    if (!out) throw runtime_error("cannot open output CSV file: " + path);
    writeResultsCsv(out, analysis);
    out.flush();
    if (!out) throw runtime_error("failed while writing output CSV file: " + path);
}

} // namespace

// ============================================================================
// Public API: reading
// ============================================================================
Circuit readCircuitCsv(istream& input, const string& sourceName) {
    string line;
    size_t lineNumber = 0;
    bool headerRead = false;
    set<string> names;
    set<int> usedNodes;
    vector<unique_ptr<Component>> components;

    while (getline(input, line)) {
        ++lineNumber;
        if (lineNumber == 1 && line.compare(0, 3, "\xEF\xBB\xBF") == 0)
            line.erase(0, 3);                              // UTF-8 BOM added by Excel

        const string content = trim(line);
        if (content.empty() || content[0] == '#') continue;

        try {
            const vector<string> fields = parseCsvRow(line);
            if (!headerRead) {
                checkHeader(fields);
                headerRead = true;
            } else {
                components.push_back(parseComponentRow(fields, names, usedNodes));
            }
        } catch (const exception& error) {
            throw CsvError(sourceName, lineNumber, error.what());
        }
    }

    if (input.bad()) throw CsvError(sourceName, 0, "failed while reading");
    if (!headerRead) throw CsvError(sourceName, 0, "missing header: type,name,node_a,node_b,value");
    if (components.empty()) throw CsvError(sourceName, 0, "the file contains no components");

    // every node from 0 to the highest must be used, otherwise the solver sees a floating node
    const int highest = *usedNodes.rbegin();
    for (int node = 0; node <= highest; ++node) {
        if (usedNodes.count(node)) continue;
        throw CsvError(sourceName, 0, node == 0
            ? string("no component is connected to node 0 (ground)")
            : "node " + to_string(node) + " is not used by any component "
              "(node numbers must have no gaps)");
    }

    Circuit result(highest + 1);
    for (auto& component : components)
        result.addComponent(move(component));
    return result;
}

Circuit readCircuitCsvFile(const string& path) {
    ifstream input(path);
    if (!input) throw CsvError(path, 0, "cannot open file");
    return readCircuitCsv(input, path);
}

// ============================================================================
// Public API: writing
// ============================================================================
void writeResultsCsv(ostream& output, const DCAnalysis& analysis) {
    requireResults(analysis);
    const auto& voltages = analysis.getResult();

    FormatGuard guard(output);
    writeHeader(output);
    for (size_t node = 0; node < voltages.size(); ++node)
        writeRow(output, 0.0, node, voltages[node]);
    if (!output) throw runtime_error("failed while writing DC results CSV");
}

void writeResultsCsv(ostream& output, const ACAnalysis& analysis) {
    requireResults(analysis);
    const auto& frequencies = analysis.getFrequencies();
    const auto& results = analysis.getResults();

    FormatGuard guard(output);
    writeHeader(output);
    for (size_t i = 0; i < frequencies.size(); ++i)
        for (size_t node = 0; node < results[i].size(); ++node)
            writeRow(output, frequencies[i], node, results[i][node]);
    if (!output) throw runtime_error("failed while writing AC results CSV");
}

void writeResultsCsvFile(const string& path, const DCAnalysis& analysis) { writeToFile(path, analysis); }
void writeResultsCsvFile(const string& path, const ACAnalysis& analysis) { writeToFile(path, analysis); }
