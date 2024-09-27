#include "integrators.h"


void IntegratorEuler::step(ParticleSystem &system, double dt) {
    Vecd x0 = system.getState();
    Vecd dx = system.getDerivative();
    Vecd x1 = x0 + dt*dx;
    system.setState(x1);
}


void IntegratorSymplecticEuler::step(ParticleSystem &system, double dt) {
}


void IntegratorMidpoint::step(ParticleSystem &system, double dt) {
    Vecd x0 = system.getState();
    Vecd k1 = system.getDerivative();
    Vecd x1 = x0 + dt*k1;
    system.setState(x1);
    Vecd k2 = system.getDerivative();
    Vecd x2 = x0 + (dt/2)*(k1+k2);
    system.setState(x2);
}

void IntegratorVerlet::step(ParticleSystem &system, double dt) {
    // TODO
}
