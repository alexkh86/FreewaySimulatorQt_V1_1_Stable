#include "Simulation.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

int SimulationConfig::locationToCell(double km) const {
    return std::clamp(static_cast<int>(std::floor(km/cellLength)),0,nCells-1);
}
void SimulationConfig::finalize() {
    if(freewayLength<=0||cellLength<=0||nLanes<1||dt<=0||simTime<=0) throw std::runtime_error("Invalid geometry/time settings.");
    if(vFree<=0||w<=0||rhoJam<=0) throw std::runtime_error("Traffic parameters must be positive.");
    nCells=std::max(1,(int)std::lround(freewayLength/cellLength));
    cellLength=freewayLength/nCells;
    nSteps=(int)std::floor(simTime/dt)+1;
    rhoCritical=w*rhoJam/(vFree+w);
    capacityLane=vFree*rhoCritical;
    capacity=capacityLane*nLanes;
    if(dt/3600.0>cellLength/std::max(vFree,w)) throw std::runtime_error("CTM CFL condition violated: reduce dt or increase cell length.");
    std::sort(demandProfile.begin(),demandProfile.end(),[](auto&a,auto&b){return a.timeHour<b.timeHour;});
    if(demandProfile.size()<2) throw std::runtime_error("Demand needs at least two points.");
    for(auto&r:onRamps){if(r.locationKm<0||r.locationKm>freewayLength)throw std::runtime_error("On-ramp outside freeway.");r.cell=locationToCell(r.locationKm);}
    for(auto&r:offRamps){if(r.beta<0||r.beta>=1)throw std::runtime_error("Off-ramp fraction must be in [0,1).");r.cell=locationToCell(r.locationKm);}
    for(auto&b:bottlenecks){if(b.capacityFactor<=0||b.capacityFactor>1)throw std::runtime_error("Bottleneck factor must be in (0,1].");b.cell=locationToCell(b.locationKm);}
    for(auto&i:incidents){
        if(i.locationKm<0||i.locationKm>freewayLength||i.lane<0||i.lane>nLanes||i.durationMin<=0||i.capacityFactor<0||i.capacityFactor>1)
            throw std::runtime_error("Invalid incident.");
        i.cell=locationToCell(i.locationKm); i.endHour=i.startHour+i.durationMin/60.0;
    }
    detectorCells.clear();
    for(double x:detectorKm){if(x<0||x>freewayLength)throw std::runtime_error("Sensor outside freeway.");detectorCells.push_back(locationToCell(x));}
}
CTMEngine::CTMEngine(){ SimulationConfig c; setConfig(c); }
CTMEngine::CTMEngine(SimulationConfig c){ setConfig(std::move(c)); }
void CTMEngine::setConfig(SimulationConfig c){c.finalize();cfg_=std::move(c);reset();}
void CTMEngine::reset(){
    const double nan=std::numeric_limits<double>::quiet_NaN();
    state_={}; state_.time.resize(cfg_.nSteps);
    for(int k=0;k<cfg_.nSteps;k++)state_.time[k]=k*cfg_.dt;
    state_.density.assign(cfg_.nSteps,std::vector<double>(cfg_.nCells,nan));
    state_.speed.assign(cfg_.nSteps,std::vector<double>(cfg_.nCells,nan));
    state_.flow.assign(cfg_.nSteps,std::vector<double>(cfg_.nCells,nan));
    for(int i=0;i<cfg_.nCells;i++){state_.density[0][i]=cfg_.initialDensity;state_.speed[0][i]=cfg_.vFree;state_.flow[0][i]=cfg_.initialDensity*cfg_.vFree*cfg_.nLanes;}
}
double CTMEngine::demand(double sec)const{
    double h=sec/3600.;auto&p=cfg_.demandProfile;
    if(h<=p.front().timeHour)return std::max(0.,p.front().demand);
    if(h>=p.back().timeHour)return std::max(0.,p.back().demand);
    for(size_t j=1;j<p.size();j++)if(h<=p[j].timeHour){auto&a=p[j-1];auto&b=p[j];double u=(h-a.timeHour)/(b.timeHour-a.timeHour);return std::max(0.,a.demand+u*(b.demand-a.demand));}
    return 0;
}
std::vector<double> CTMEngine::incidentFactors(double sec,std::vector<bool>&active)const{
    active.assign(cfg_.incidents.size(),false);std::vector<double>f(cfg_.nCells,1.0);double h=sec/3600.;
    for(size_t j=0;j<cfg_.incidents.size();j++){auto&i=cfg_.incidents[j];if(h>=i.startHour&&h<i.endHour){active[j]=true;double z=i.lane==0?i.capacityFactor:((cfg_.nLanes-1)+i.capacityFactor)/cfg_.nLanes;f[i.cell]=std::min(f[i.cell],z);}}
    return f;
}
std::vector<bool> CTMEngine::activeIncidents(double sec)const{std::vector<bool>a;incidentFactors(sec,a);return a;}
bool CTMEngine::step(){
    int k=state_.currentStep;if(k>=cfg_.nSteps-1){state_.finished=true;return false;}
    int N=cfg_.nCells;double dt=cfg_.dt/3600.,L=cfg_.cellLength,t=state_.time[k];
    std::vector<double>rt(N),cap(N,cfg_.capacity),S(N),R(N),beta(N),rampOn(N),q(N+1),off(N),on(N);
    for(int i=0;i<N;i++)rt[i]=state_.density[k][i]*cfg_.nLanes;
    for(auto&b:cfg_.bottlenecks)cap[b.cell]=std::min(cap[b.cell],cfg_.capacity*b.capacityFactor);
    std::vector<bool>active;auto fi=incidentFactors(t,active);for(int i=0;i<N;i++)cap[i]*=fi[i];
    for(int i=0;i<N;i++){S[i]=std::min(cfg_.vFree*rt[i],cap[i]);R[i]=std::max(0.,std::min(cap[i],cfg_.w*(cfg_.rhoJam*cfg_.nLanes-rt[i])));}
    double progress=std::clamp(t/cfg_.simTime,0.,1.),shape=std::pow(std::sin(3.14159265358979323846*progress),2);
    for(auto&r:cfg_.onRamps)rampOn[r.cell]+=r.lowDemand+(r.highDemand-r.lowDemand)*shape;
    for(auto&r:cfg_.offRamps)beta[r.cell]=std::min(.95,beta[r.cell]+r.beta);
    q[0]=std::min(demand(t),R[0]);
    for(int i=0;i<N-1;i++){double accepted=std::min(S[i],R[i+1]/std::max(1.-beta[i],1e-12));q[i+1]=(1-beta[i])*accepted;off[i]=beta[i]*accepted;}
    q[N]=(1-beta[N-1])*S[N-1];off[N-1]=beta[N-1]*S[N-1];
    for(int i=0;i<N;i++)if(rampOn[i]>0)on[i]=std::min(rampOn[i],std::max(0.,R[i]-q[i]));
    for(int i=0;i<N;i++){
        double total=rt[i]+dt/L*(q[i]+on[i]-q[i+1]-off[i]);
        double rho=std::clamp(total/cfg_.nLanes,0.,cfg_.rhoJam),v=cfg_.vFree;
        if(rho>cfg_.rhoCritical)v=cfg_.w*(cfg_.rhoJam-rho)/std::max(rho,1e-9);
        v=std::clamp(v,cfg_.minSpeed,cfg_.vFree);
        state_.density[k+1][i]=rho;state_.speed[k+1][i]=v;state_.flow[k+1][i]=std::min(rho*v*cfg_.nLanes,cap[i]);
    }
    state_.currentStep++;if(state_.currentStep>=cfg_.nSteps-1)state_.finished=true;return true;
}
void CTMEngine::advanceSeconds(double sec){int n=std::max(0,(int)std::lround(sec/cfg_.dt));while(n--&&!state_.finished)step();}
