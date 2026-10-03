#pragma once

#include <iosfwd>
#include <string>
#include "analysis.h"

Circuit readCircuitCsv(std::istream& input, const std::string& sourceName = "<stream>");
Circuit readCircuitCsvFile(const std::string& path);

void writeDCResultsCsv(std::ostream& output, const DCAnalysis& analysis);
void writeACResultsCsv(std::ostream& output, const ACAnalysis& analysis);
void writeDCResultsCsvFile(const std::string& path, const DCAnalysis& analysis);
void writeACResultsCsvFile(const std::string& path, const ACAnalysis& analysis);
