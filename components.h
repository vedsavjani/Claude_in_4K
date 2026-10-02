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
    Component(string n, int a, int b) : name(n), nodeA(a), nodeB(b) {}

    // virtual destructor
    virtual ~Component() = default;

    // some getter functions
    virtual complex<double> getImpedance(double frequency) const = 0;
    virtual double getValue() const = 0; // Ohms for resistor, Farads for capacitor, Henry for inductor
    virtual string getType() const = 0; // gives "R" for resistor, "C" for capacitor, "L" for inductor,
                                        // useful for printing results and debugging
                                    
    // these three getters dont use virtual becoz the their behavious is same for all components
    string getName() const {
        return name;
    }               

    int getNodeA() const{
        return nodeA;
    }

    int getNodeB() const{
        return nodeB;
    }
};

class Resistor : public Component {
private:
    double resistance;
public:
    Resistor(string n, int a, int b, double r) : Component(n, a, b) , resistance(r) {}

    // overriding some of the getter functions in Component class
    complex<double> getImpedance(double frequency) const override {
        return complex<double>(resistance, 0.0);
    }

    double getValue() const override {
        return resistance;
    }

    string getType() const override {
        return "R";
    }
};

class Inductor : public Component {
private:
    double inductance;
public:
    Inductor(string n, int a, int b, double l) : Component(n, a, b) , inductance(l) {}

    // overriding some of the getter functions in Component class
    complex<double> getImpedance(double frequency) const override {
        return complex<double>(0.0, 2*PI*frequency*inductance);    // Z_l = jwL = j*2*pi*f*L
    }

    double getValue() const override {
        return inductance;
    }

    string getType() const override {
        return "L";
    }
};

class Capacitor : public Component {
private:
    double capacitance;
public:
    Capacitor(string n, int a, int b, double f) : Component(n, a, b) , capacitance(f) {}

    // overriding some of the getter functions in Component class
    complex<double> getImpedance(double frequency) const override {
        return complex<double>(0.0, -1/(2*PI*frequency*capacitance));    // Z_c = 1/jwC = -j/wC = -j/2*pi*f*C
    }

    double getValue() const override {
        return capacitance;
    }

    string getType() const override {
        return "C";
    }
};

class VoltageSource : public Component {
private:
    double voltage; // this is an ideal, independent voltage source
    double freq; 
public:
    VoltageSource(string n, int a, int b, double v, double fr) : Component(n, a, b) , voltage(v), freq(fr) {}

    double getFrequency() const {
        return freq;
    }

    // overriding some of the getter functions in Component class
    complex<double> getImpedance(double frequency) const override {
        return complex<double>(0.0, 0.0);    // impedence is zero for an ideal voltage source
    }

    double getValue() const override {
        return voltage;
    }

    string getType() const override {
        return "V";
    }
};