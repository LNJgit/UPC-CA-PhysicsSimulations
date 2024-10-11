#include "integrators.h"


void IntegratorEuler::step(ParticleSystem &system, double dt) {
    Vecd x0 = system.getState();
    Vecd dx = system.getDerivative();
    Vecd x1 = x0 + dt*dx;
    system.setState(x1);
}


void IntegratorSymplecticEuler::step(ParticleSystem &system, double dt) {
    Vecd current_velocity = system.getVelocities();
    Vecd next_velocity = current_velocity + system.getAccelerations()*dt;
    Vecd next_position = system.getPositions() + dt*next_velocity;
    system.updateForces();
    system.setPositions(next_position);
    system.setVelocities(next_velocity);

}

void IntegratorRK2::step(ParticleSystem &system, double dt) {
    Vecd x0 = system.getState();
    Vecd k1 = system.getDerivative();
    Vecd x1 = x0 + dt*k1;
    system.setState(x1);
    Vecd k2 = system.getDerivative();
    Vecd x2 = x0 + (dt/2)*(k1+k2);
    system.setState(x2);
}

void IntegratorMidpoint::step(ParticleSystem &system, double dt) {
    Vecd x0 = system.getState();
    Vecd dx = system.getDerivative();
    Vecd xmid = x0 + (dt*dx)/2;
    system.setState(xmid);
    Vecd fmid = system.getDerivative();
    Vecd x1 = x0 + dt*fmid;
    system.setState(x1);


}

void IntegratorVerlet::step(ParticleSystem &system, double dt) {
    Vecd current_position = system.getPositions();
    Vecd current_velocity = system.getVelocities();

    Vecd next_position = current_position + (dt*current_velocity) + pow(dt,2.0)*system.getAccelerations();
    system.updateForces();
    system.setPositions(next_position);
    system.setVelocities((next_position-current_position)/dt);

}
