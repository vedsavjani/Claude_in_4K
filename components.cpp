#include "components.h"

// ---------- Component ----------
Component::Component(string n, int a, int b) : name(n), nodeA(a), nodeB(b) {}

string Component::getName() const { return name; }
int Component::getNodeA() const { return nodeA; }
int Component::getNodeB() const { return nodeB; }

// ---------- Resistor ----------
Resistor::Resistor(string n, int a, int b, double r) : Component(n, a, b), resistance(r) {}

complex<double> Resistor::getImpedance(double frequency) const {
    return {resistance, 0.0};
}
double Resistor::getValue() const { return resistance; }
string Resistor::getType() const { return "R"; }

// ---------- Inductor ----------
Inductor::Inductor(string n, int a, int b, double l) : Component(n, a, b), inductance(l) {}

complex<double> Inductor::getImpedance(double frequency) const {
    return {0.0, 2 * PI * frequency * inductance};    // Z_l = jwL = j*2*pi*f*L
}
double Inductor::getValue() const { return inductance; }
string Inductor::getType() const { return "L"; }

// ---------- Capacitor ----------
Capacitor::Capacitor(string n, int a, int b, double c) : Component(n, a, b), capacitance(c) {}

complex<double> Capacitor::getImpedance(double frequency) const {
    return {0.0, -1.0 / (2 * PI * frequency * capacitance)};    // Z_c = 1/jwC = -j/wC = -j/2*pi*f*C
}
double Capacitor::getValue() const { return capacitance; }
string Capacitor::getType() const { return "C"; }

// ---------- VoltageSource ----------
VoltageSource::VoltageSource(string n, int a, int b, double v, double fr)
    : Component(n, a, b), voltage(v), freq(fr) {}

double VoltageSource::getFrequency() const { return freq; }

complex<double> VoltageSource::getImpedance(double frequency) const {
    return {0.0, 0.0};    // impedance is zero for an ideal voltage source
}
double VoltageSource::getValue() const { return voltage; }
string VoltageSource::getType() const { return "V"; }