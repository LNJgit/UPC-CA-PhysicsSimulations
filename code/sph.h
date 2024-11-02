#ifndef SPH_H
#define SPH_H

#include "defines.h"
#include "particle.h"


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
    SPH() : cellSize(2.0), smoothingLength(0.5), restDensity(1000.0), viscosity(0.001) {}
    SPH(double cellSize, double smoothingLength, double restDensity, double viscosity);
    ~SPH() {};



    std::unordered_map<int, std::vector<ParticleSPH>> particleMap;
    std::vector<ParticleSPH> Particles;

    int getHashValue(Particle* p) const;
    void addParticle(Particle* p) ;
    void computeDensity(ParticleSPH& particle);
    void computePressure(ParticleSPH& particle);
    void computeAcceleration(ParticleSPH& particle);
    void clearParticles();
    void getNeighbors(ParticleSPH& particle);
    int getHashValueForOffset(const Particle* p, int dx, int dy, int dz) const;
    void setCellSize(int size);
    void setSmoothingLength(float radius);
    void computePressureForces(ParticleSPH& particle);
    void initializeReferenceDensity();
    double getSmoothingLength();
    double getRestingDensity();


protected:
    double cellSize;
    double smoothingLength;       // The distance within which particles affect each other
    double restDensity;           // Desired rest density for the fluid
    double viscosity;             // Fluid viscosity constant


    // Kernel functions
    double smoothingKernel(double r, double h);
    double smoothingKernelDerivative(double r, double h);
    Vec3 smoothingKernelGradient(const ParticleSPH& p1, const ParticleSPH& p2);
    double viscosityKernelLaplacian(double r, double h);
};

#endif // SPH_H
