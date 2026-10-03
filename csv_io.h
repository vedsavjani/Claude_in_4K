#pragma once

#include <cstddef>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include "analysis.h"

// Thrown for any problem with a circuit CSV file. what() looks like
// "my_circuit.csv:4: duplicate component name: R1".
class CsvError : public std::runtime_error {
    std::string source_;
    std::size_t line_;
public:
    CsvError(const std::string& source, std::size_t line, const std::string& message);
    const std::string& source() const { return source_; }
    std::size_t line() const { return line_; }     // 0 = not tied to one line
};

// ---------------- input: circuit description ----------------
// Format (header is required, '#' starts a comment line):
//     type,name,node_a,node_b,value
//     V,V1,1,0,5
//     R,R1,1,2,4.7k          <- values may use SPICE suffixes: f p n u m k meg g t
// Types: R, L, C, V.  Node 0 is ground.  Node numbers must have no gaps.
Circuit readCircuitCsv(std::istream& input, const std::string& sourceName = "<stream>");
Circuit readCircuitCsvFile(const std::string& path);

// ---------------- output: analysis results ----------------
// One tidy table for both analyses:
//     frequency_hz,node,voltage_real_v,voltage_imag_v,magnitude_v,phase_deg
// (DC rows simply have frequency_hz = 0). Throws std::logic_error if run() was not called.
void writeResultsCsv(std::ostream& output, const DCAnalysis& analysis);
void writeResultsCsv(std::ostream& output, const ACAnalysis& analysis);

// File versions: an existing file is only replaced if the whole export succeeds.
void writeResultsCsvFile(const std::string& path, const DCAnalysis& analysis);
void writeResultsCsvFile(const std::string& path, const ACAnalysis& analysis);
