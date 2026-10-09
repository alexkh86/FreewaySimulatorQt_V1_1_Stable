#pragma once
#include <vector>
#include <cstddef>

struct DemandPoint { double timeHour{}, demand{}; };
struct OnRamp { double locationKm{}, lowDemand{}, highDemand{}; int cell{}; };
struct OffRamp { double locationKm{}, beta{}; int cell{}; };
struct Bottleneck { double locationKm{}, capacityFactor{1}; int cell{}; };
struct Incident {
    double locationKm{}; int lane{};
    double startHour{}, durationMin{}, capacityFactor{1}, endHour{};
    int cell{};
};

struct SimulationConfig {
    double freewayLength=20, cellLength=.5;
    int nLanes=3;
    double dt=5, simTime=3*3600, vFree=100, w=20, rhoJam=150;
    double initialDensity=10, minSpeed=5;
    std::vector<DemandPoint> demandProfile{{0,2500},{.5,2500},{1,7000},{2,7000},{2.5,5000},{3,5000}};
    std::vector<OnRamp> onRamps{{8,200,900}};
    std::vector<OffRamp> offRamps{{15,.15}};
    std::vector<Bottleneck> bottlenecks{{12,.75}};
    std::vector<Incident> incidents{{10,2,1.25,20,0}};
    std::vector<double> detectorKm{2.5,6,10,14,18};
    int nCells{}, nSteps{};
    double rhoCritical{}, capacityLane{}, capacity{};
    std::vector<int> detectorCells;
    void finalize();
    int locationToCell(double km) const;
};

struct SimulationState {
    std::vector<double> time;
    std::vector<std::vector<double>> density,speed,flow;
    int currentStep=0;
    bool finished=false;
};

class CTMEngine {
public:
    CTMEngine();
    explicit CTMEngine(SimulationConfig cfg);
    void setConfig(SimulationConfig cfg);
    void reset();
    bool step();
    void advanceSeconds(double seconds);
    double demand(double seconds) const;
    std::vector<bool> activeIncidents(double seconds) const;
    const SimulationConfig& config() const { return cfg_; }
    const SimulationState& state() const { return state_; }
private:
    SimulationConfig cfg_;
    SimulationState state_;
    std::vector<double> incidentFactors(double seconds,std::vector<bool>& active) const;
};
