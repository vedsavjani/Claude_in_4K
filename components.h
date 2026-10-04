#pragma once
#include <complex>
#include <string>
using namespace std;

const double PI = 3.14159265359;

class Component {
protected:
    string name;
    int nodeA;
    int nodeB;
public:
    Component(string n, int a, int b);

    // virtual destructor
    virtual ~Component() = default;

    // some getter functions
    virtual complex<double> getImpedance(double frequency) const = 0;
    virtual double getValue() const = 0;  // Ohms for resistor, Farads for capacitor, Henry for inductor, Volts for voltage source
    virtual string getType() const = 0;   // gives "R" for resistor, "C" for capacitor, "L" for inductor, "V" for voltage source
                                          // useful for printing results and debugging

    // these three getters dont use virtual becoz their behaviour is same for all components
    string getName() const;
    int getNodeA() const;
    int getNodeB() const;
};

class Resistor : public Component {
    double resistance;
public:
    Resistor(string n, int a, int b, double r);

    // overriding some of the getter functions in Component class
    complex<double> getImpedance(double frequency) const override;
    double getValue() const override;
    string getType() const override;
};

class Inductor : public Component {
    double inductance;
public:
    Inductor(string n, int a, int b, double l);

    // overriding some of the getter functions in Component class
    complex<double> getImpedance(double frequency) const override;
    double getValue() const override;
    string getType() const override;
};

class Capacitor : public Component {
    double capacitance;
public:
    Capacitor(string n, int a, int b, double c);

    // overriding some of the getter functions in Component class
    complex<double> getImpedance(double frequency) const override;  // assumes f > 0
    double getValue() const override;
    string getType() const override;
};

class VoltageSource : public Component {
    double voltage; // this is an ideal, independent voltage source
    double freq;
public:
    VoltageSource(string n, int a, int b, double v, double fr);
    double getFrequency() const;

    // overriding some of the getter functions in Component class
    complex<double> getImpedance(double frequency) const override;
    double getValue() const override;
    string getType() const override;
};