#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include "csv_io.h"

int passed = 0, failed = 0;

void check(const string& test, bool ok, const string& detail = "") {
    if (ok) { passed++; cout << "PASS  " << test << "\n"; }
    else    { failed++; cout << "FAIL  " << test << "  " << detail << "\n"; }
}

const string H = "type,name,node_a,node_b,value\n";

Circuit parse(const string& text) { istringstream in(text); return readCircuitCsv(in, "test.csv"); }

// the text must be rejected, and the message must contain `expected`
void expectError(const string& test, const string& text, const string& expected) {
    try { parse(text); check(test, false, "(no error thrown)"); }
    catch (const CsvError& e) {
        string msg = e.what();
        check(test, msg.find(expected) != string::npos, "got: " + msg);
    }
}

vector<string> split(const string& line) {
    vector<string> out; string f; istringstream s(line);
    while (getline(s, f, ',')) out.push_back(f);
    return out;
}

struct CommaDecimal : numpunct<char> { char do_decimal_point() const override { return ','; } };

int main() {
    // ---------- reading: valid input ----------
    {
        Circuit c = parse(H + "V,V1,1,0,5\nR,R1,1,2,4700\nL,L1,2,3,0.01\nC,C1,3,0,1e-6\n");
        check("reads 4 components", c.getComponents().size() == 4);
        check("derives node count (4)", c.getNumNodes() == 4);
        check("plain number 4700", abs(c.getComponents()[1]->getValue() - 4700) < 1e-9);
        check("decimal 0.01", abs(c.getComponents()[2]->getValue() - 0.01) < 1e-12);
        check("scientific 1e-6", abs(c.getComponents()[3]->getValue() - 1e-6) < 1e-15);
        check("explicit + sign", abs(parse(H + "V,V1,1,0,1\nR,R1,1,0,+100\n").getComponents()[1]->getValue() - 100) < 1e-9);
    }
    check("CRLF, spaces and comments", parse("# c\r\n type , name , node_a , node_b , value \r\n V , V1 , 1 , 0 , 5 \r\nR,R1,1,0,1000\r\n").getComponents().size() == 2);
    check("quoted names", parse(H + "V,\"V,1\",1,0,5\nR,\"R \"\"x\"\"\",1,0,1000\n").getComponents()[1]->getName() == "R \"x\"");
    check("Excel BOM accepted", parse(string("\xEF\xBB\xBF") + H + "V,V1,1,0,5\nR,R1,1,0,1000\n").getComponents().size() == 2);
    check("header is case-insensitive", parse("TYPE,Name,NODE_A,node_b,Value\nv,V1,1,0,5\nr,R1,1,0,100\n").getComponents().size() == 2);

    // ---------- reading: bad input (message must say what and where) ----------
    expectError("duplicate name", H + "V,V1,1,0,5\nR,V1,1,0,1000\n", "test.csv:3: duplicate component name: V1");
    expectError("negative node", H + "V,V1,1,0,5\nR,R1,1,-2,1000\n", "test.csv:3: node must be");
    expectError("non-numeric node", H + "V,V1,1,0,5\nR,R1,1,x,1000\n", "test.csv:3: node must be");
    expectError("huge node", H + "V,V1,1,0,5\nR,R1,1,2000000000,1000\n", "too large");
    expectError("negative resistance", H + "V,V1,1,0,5\nR,R1,1,0,-5\n", "greater than zero");
    expectError("zero capacitance", H + "V,V1,1,0,5\nC,C1,1,0,0\n", "greater than zero");
    expectError("same node twice", H + "V,V1,1,0,5\nR,R1,1,1,100\n", "both terminals of R1 are on node 1");
    expectError("bad value", H + "V,V1,1,0,5\nR,R1,1,0,abc\n", "not a valid number");
    expectError("suffix 4.7k rejected", H + "V,V1,1,0,5\nR,R1,1,0,4.7k\n", "suffixes like k or u are not supported");
    expectError("suffix 10u rejected", H + "V,V1,1,0,5\nC,C1,1,0,10u\n", "not a valid number");
    expectError("hex rejected", H + "V,V1,1,0,5\nR,R1,1,0,0x10\n", "not a valid number");
    expectError("nan rejected", H + "V,V1,1,0,5\nR,R1,1,0,nan\n", "not a valid number");
    expectError("incomplete exponent", H + "V,V1,1,0,5\nR,R1,1,0,5e\n", "not a valid number");
    expectError("out-of-range value", H + "V,V1,1,0,5\nR,R1,1,0,1e999\n", "not a valid number");
    expectError("inf value", H + "V,V1,1,0,5\nR,R1,1,0,inf\n", "not a valid number");
    expectError("unknown type", H + "V,V1,1,0,5\nX,Q1,1,0,1\n", "unsupported component type 'X'");
    expectError("wrong field count", H + "V,V1,1,0\n", "expected 5 fields");
    expectError("missing header", "V,V1,1,0,5\n", "expected header");
    expectError("header only", H, "no components");
    expectError("empty file", "", "missing header");
    expectError("unterminated quote", H + "V,\"V1,1,0,5\n", "unterminated quoted field");
    expectError("node gap", H + "V,V1,1,0,5\nR,R1,1,50,1000\nR,R2,50,0,1000\n", "node 2 is not used");
    expectError("no ground", H + "V,V1,1,2,5\nR,R1,1,2,1000\n", "node 0 (ground)");
    try { readCircuitCsvFile("/no/such/file.csv"); check("missing input file", false); }
    catch (const CsvError& e) { check("missing input file", string(e.what()).find("cannot open file") != string::npos); }

    // ---------- writing: round trip through the real solver ----------
    NodalSimulator sim;
    Circuit rlc = parse(H + "V,V1,1,0,5\nR,R1,1,2,100\nL,L1,2,3,0.01\nC,C1,3,0,1e-6\n");
    DCAnalysis dc(sim);
    ACAnalysis ac(sim, 1000, 2000, 500);

    { ostringstream o; try { writeResultsCsv(o, dc); check("DC export before run() fails", false); }
      catch (const logic_error&) { check("DC export before run() fails", o.str().empty()); } }
    { ostringstream o; try { writeResultsCsv(o, ac); check("AC export before run() fails", false); }
      catch (const logic_error&) { check("AC export before run() fails", o.str().empty()); } }

    dc.run(rlc); ac.run(rlc);

    {
        ostringstream o; writeResultsCsv(o, ac);
        istringstream in(o.str()); string line; getline(in, line);
        check("AC header", line == "frequency_hz,node,voltage_real_v,voltage_imag_v,magnitude_v,phase_deg");
        int rows = 0; bool numbersMatch = true;
        while (getline(in, line)) {
            auto f = split(line);
            if (f.size() != 6) { numbersMatch = false; break; }
            double hz = stod(f[0]); int node = stoi(f[1]);
            size_t fi = (size_t)((hz - 1000) / 500);
            complex<double> got(stod(f[2]), stod(f[3])), want = ac.getResults()[fi][node];
            if (abs(got - want) > 1e-9) numbersMatch = false;
            rows++;
        }
        check("AC: 3 freqs x 4 nodes = 12 rows", rows == 12, "rows=" + to_string(rows));
        check("AC: CSV values equal solver values (1e-9)", numbersMatch);
    }
    {
        ostringstream o; writeResultsCsv(o, dc);
        check("DC: no negative zeros in output", o.str().find("-0,") == string::npos && o.str().find("-0\n") == string::npos, o.str());
        check("DC: full precision (inductor stamp visible)", o.str().find("5.00000476832613") != string::npos, o.str());
    }
    {   // caller's stream settings must survive
        ostringstream o; o << fixed << setprecision(2);
        writeResultsCsv(o, dc);
        o.str(""); o << 3.14159;
        check("stream format restored", o.str() == "3.14", o.str());
    }
    {   // a comma-decimal locale must not break the file
        ostringstream o; o.imbue(locale(locale::classic(), new CommaDecimal));
        writeResultsCsv(o, ac);
        istringstream in(o.str()); string line; getline(in, line); bool ok = true;
        while (getline(in, line)) if (split(line).size() != 6) ok = false;
        check("comma-decimal locale -> still 6 columns", ok);
    }

    // ---------- files ----------
    namespace fs = filesystem;
    fs::path dir = fs::temp_directory_path() / "minispice_csv_test";
    fs::create_directories(dir);
    string good = (dir / "ac.csv").string(), keep = (dir / "keep.csv").string();
    writeResultsCsvFile(good, ac);
    check("file written", fs::exists(good));
    { ofstream f(keep); f << "precious,data\n"; }
    DCAnalysis notRun(sim);
    try { writeResultsCsvFile(keep, notRun); check("failed export throws", false); }
    catch (const logic_error&) {
        ifstream f(keep); string l; getline(f, l);
        check("failed export leaves existing file untouched", l == "precious,data");
    }
    try { writeResultsCsvFile((dir / "no_such_dir" / "x.csv").string(), dc); check("bad output path throws", false); }
    catch (const runtime_error&) { check("bad output path throws", true); }
    fs::remove_all(dir);

    cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
