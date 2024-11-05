#ifndef SPH_H
#define SPH_H

#include "defines.h"
#include "particle.h"
#include <unordered_map>
#include <vector>

struct ParticleSPH {
    Particle* p;
    double density;
    double pressure;
    Vec3 forces;
    std::vector<ParticleSPH*> neighbors;
};

class SPH
{
public:
    SPH() : cellSize(2.0), smoothingLength(0.5) {}
    SPH(double cellSize, double smoothingLength);
    ~SPH() {}

    std::unordered_map<int, std::vector<ParticleSPH>> particleMap;
    std::vector<ParticleSPH> Particles;

    int getHashValue(Particle* p) const;
    int getHashValueForOffset(const Particle* p, int dx, int dy, int dz) const;
    void addParticle(Particle* p);
    void clearParticles();
    void getNeighbors(ParticleSPH& particle);

    void computeDensity(ParticleSPH& particle);
    void computePressure(ParticleSPH& particle, double restDensity, double cs);
    void computeAcceleration(ParticleSPH& particle);
    void computePressureForces(ParticleSPH& particle, double viscosity);

    double initializeReferenceDensity();
    double getSmoothingLength();
    double getRestingDensity();

    void setCellSize(int size);
    void setSmoothingLength(float radius);

protected:
    double cellSize;
    double smoothingLength;

    double smoothingKernel(double r, double h);
    double smoothingKernelDerivative(double r, double h);
    Vec3 smoothingKernelGradient(const ParticleSPH& p1, const ParticleSPH& p2);
    double viscosityKernelLaplacian(double r, double h);
    double viscosityKernel(double r, double h);
};

#endif // SPH_H
