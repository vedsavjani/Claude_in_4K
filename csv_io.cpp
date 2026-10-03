#include "csv_io.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
// #include <limits>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

using namespace std;

namespace {  //internal namespace , to keep this part private to this source .

string trim(const string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");   //to trim the extra whitespaces in beginning (ex- "    hello"->"hello")
    if (first == string::npos)    //string::npos == not found
        return "";
    const auto last = value.find_last_not_of(" \t\r\n");   //for ending
    return value.substr(first, last - first + 1);  //extracting the substring
}

vector<string> parseCsvRow(const string& line) {
    vector<string> fields;
    string field;
    bool quoted = false;
    bool quoteClosed = false;

    for (int i = 0; i < line.size(); i++) {
        const char ch = line[i];
        if (quoted) {
            if (ch == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    ++i;
                } else {
                    quoted = false;
                    quoteClosed = true;
                }
            } else {
                field += ch;
            }
        } else if (ch == ',') {
            fields.push_back(trim(field));   //putting the string to the vector.
            field.clear();
            quoteClosed = false;
        } else if (ch == '"') {
            if (!trim(field).empty() || quoteClosed)   //Quotes are only valid at start of a field
                throw invalid_argument("unexpected quote in unquoted field");
            field.clear();    
            quoted = true;
        } else {
            if (quoteClosed && ch != ' ' && ch != '\t' && ch != '\r')
                throw invalid_argument("unexpected text after quoted field");
            if (!quoteClosed)
                field += ch;
        }
    }

    if (quoted)        //If a quote was opened and never closed, error
        throw invalid_argument("unterminated quoted field");
    fields.push_back(trim(field));  //Save the last field and return the list
    return fields;
}

int parseNode(const string& value) {   //convert int in string to int (like "3" -> 3)
    size_t parsed = 0;
    long long node;
    try {
        node = stoll(value, &parsed);  //converts string to long long ..
    } catch (const exception&) {  //if fail.
        throw invalid_argument("node must be a non-negative integer");
    }
    if (parsed != value.size() || node < 0 )
        throw invalid_argument("node must be a non-negative integer");
    return int(node);
}

double parseValue(const string& value) {   //string to double
    size_t parsed = 0;
    double number;
    try {
        number = stod(value, &parsed);
    } catch (const exception&) {
        throw invalid_argument("value must be a finite number");
    }
    if (parsed != value.size() || !isfinite(number))
        throw invalid_argument("value must be a finite number");
    return number;
}

void writeResultsHeader(ostream& output) {
    output << "frequency_hz,node,voltage_real_v,voltage_imag_v,magnitude_v,phase_deg\n";
}

void writeVoltageRow(ostream& output, double frequency, size_t node,
                     const complex<double>& voltage) {
    output << frequency << ',' << node << ',' << voltage.real() << ','
           << voltage.imag() << ',' << abs(voltage) << ','
           << arg(voltage) * 180.0 / PI << '\n';
}

template <typename Writer>
void writeFile(const string& path, Writer writer) {
    ofstream output(path);
    if (!output)
        throw runtime_error("cannot open output CSV file: " + path);
    output << setprecision(15);
    writer(output);
    if (!output)
        throw runtime_error("failed while writing output CSV file: " + path);
}

} // namespace

Circuit readCircuitCsv(istream& input, const string& sourceName) {
    string line;
    size_t lineNumber = 0;
    bool headerRead = false;
    int highestNode = 0;
    set<string> names;
    vector<unique_ptr<Component>> components;

    while (getline(input, line)) {
        ++lineNumber;
        const string content = trim(line);
        if (content.empty() || content[0] == '#')
            continue;

        vector<string> fields;
        try {
            fields = parseCsvRow(line);
        } catch (const exception& error) {
            throw runtime_error(sourceName + ":" + to_string(lineNumber) +
                                     ": " + error.what());
        }

        if (!headerRead) {
            const vector<string> expected = {
                "type", "name", "node_a", "node_b", "value"
            };
            if (fields.size() != expected.size()) {
                throw runtime_error(sourceName + ":" +
                    to_string(lineNumber) +
                    ": expected header type,name,node_a,node_b,value");
            }
            for (size_t i = 0; i < expected.size(); ++i) {
                string header = fields[i];
                for (char& ch : header)
                    ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
                if (header != expected[i])
                    throw runtime_error(sourceName + ":" +
                        to_string(lineNumber) +
                        ": expected header type,name,node_a,node_b,value");
            }
            headerRead = true;
            continue;
        }

        try {
            if (fields.size() != 5)
                throw invalid_argument("expected 5 fields: type,name,node_a,node_b,value");

            string type = fields[0];
            for (char& ch : type)
                ch = static_cast<char>(toupper(static_cast<unsigned char>(ch)));
            const string& name = fields[1];
            if (name.empty())
                throw invalid_argument("component name cannot be empty");
            if (!names.insert(name).second)
                throw invalid_argument("duplicate component name: " + name);

            const int nodeA = parseNode(fields[2]);
            const int nodeB = parseNode(fields[3]);
            const double value = parseValue(fields[4]);
            if ((type == "R" || type == "L" || type == "C") && value <= 0)
                throw invalid_argument("R, L, and C values must be greater than zero");

            unique_ptr<Component> component;
            if (type == "R")
                component = make_unique<Resistor>(name, nodeA, nodeB, value);
            else if (type == "L")
                component = make_unique<Inductor>(name, nodeA, nodeB, value);
            else if (type == "C")
                component = make_unique<Capacitor>(name, nodeA, nodeB, value);
            else if (type == "V")
                component = make_unique<VoltageSource>(name, nodeA, nodeB, value, 0);
            else
                throw invalid_argument("unsupported component type: " + type);

            highestNode = max(highestNode, max(nodeA, nodeB));
            components.push_back(move(component));
        } catch (const exception& error) {
            throw runtime_error(sourceName + ":" + to_string(lineNumber) +
                                     ": " + error.what());
        }
    }

    if (!input.eof() && input.fail())
        throw runtime_error("failed while reading CSV file: " + sourceName);
    if (!headerRead)
        throw runtime_error(sourceName + ": missing CSV header");
    if (components.empty())
        throw runtime_error(sourceName + ": CSV contains no components");

    Circuit result(highestNode + 1);
    for (auto& component : components)
        result.addComponent(move(component));
    return result;
}

Circuit readCircuitCsvFile(const string& path) {
    ifstream input(path);
    if (!input)
        throw runtime_error("cannot open input CSV file: " + path);
    return readCircuitCsv(input, path);
}

void writeDCResultsCsv(ostream& output, const DCAnalysis& analysis) {
    const auto& voltages = analysis.getResult();
    if (voltages.empty())
        throw logic_error("run the DC analysis before exporting its results");
    writeResultsHeader(output);
    for (size_t node = 0; node < voltages.size(); ++node)
        writeVoltageRow(output, 0, node, voltages[node]);
    if (!output)
        throw runtime_error("failed while writing DC results CSV");
}

void writeACResultsCsv(ostream& output, const ACAnalysis& analysis) {
    const auto& frequencies = analysis.getFrequencies();
    const auto& results = analysis.getResults();
    if (frequencies.empty() || results.size() != frequencies.size())
        throw logic_error("run the AC analysis before exporting its results");
    writeResultsHeader(output);
    for (size_t i = 0; i < frequencies.size(); ++i)
        for (size_t node = 0; node < results[i].size(); ++node)
            writeVoltageRow(output, frequencies[i], node, results[i][node]);
    if (!output)
        throw runtime_error("failed while writing AC results CSV");
}

void writeDCResultsCsvFile(const string& path, const DCAnalysis& analysis) {
    writeFile(path, [&analysis](ostream& output) {
        writeDCResultsCsv(output, analysis);
    });
}

void writeACResultsCsvFile(const string& path, const ACAnalysis& analysis) {
    writeFile(path, [&analysis](ostream& output) {
        writeACResultsCsv(output, analysis);
    });
}
