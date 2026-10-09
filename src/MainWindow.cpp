#include "MainWindow.h"
#include "FreewayView.h"
#include "qcustomplot.h"
#include <QtWidgets>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <limits>
#include <stdexcept>

static QDoubleSpinBox* spin(double v,double lo,double hi,int dec=2){auto*w=new QDoubleSpinBox;w->setRange(lo,hi);w->setDecimals(dec);w->setValue(v);return w;}
static void row(QTableWidget*t,std::initializer_list<double>a){int r=t->rowCount();t->insertRow(r);int c=0;for(double x:a)t->setItem(r,c++,new QTableWidgetItem(QString::number(x)));}
static double tv(QTableWidget*t,int r,int c){if(!t->item(r,c))throw std::runtime_error("Empty table cell.");bool ok=false;double x=t->item(r,c)->text().toDouble(&ok);if(!ok)throw std::runtime_error("Invalid numeric table value.");return x;}
static QGroupBox* plotGroup(const QString&name,QCustomPlot*&p){auto*g=new QGroupBox(name);auto*l=new QVBoxLayout(g);p=new QCustomPlot;l->addWidget(p);return g;}

MainWindow::MainWindow(){
 buildUi();
 timer_=new QTimer(this);
 timer_->setSingleShot(false);
 timer_->setTimerType(Qt::CoarseTimer);
 connect(timer_,&QTimer::timeout,this,&MainWindow::tick);
 resize(1800,980);
 setupEmptyPlots();
 refreshLightweight();
}
void MainWindow::buildUi(){
 auto*cw=new QWidget;setCentralWidget(cw);auto*root=new QHBoxLayout(cw);
 auto*tabs=new QTabWidget;tabs->setMinimumWidth(390);tabs->setMaximumWidth(470);root->addWidget(tabs);
 auto*g=new QWidget;auto*f=new QFormLayout(g);L_=spin(20,.1,10000);dx_=spin(.5,.01,100);lanes_=new QSpinBox;lanes_->setRange(1,20);lanes_->setValue(3);duration_=spin(3,.01,1000);dt_=spin(5,.01,3600);vf_=spin(100,1,300);wave_=spin(20,1,200);jam_=spin(150,1,1000);sensors_=new QLineEdit("2.5,6,10,14,18");
 f->addRow("Freeway length [km]",L_);f->addRow("Cell length [km]",dx_);f->addRow("Lanes",lanes_);f->addRow("Duration [h]",duration_);f->addRow("CTM dt [s]",dt_);f->addRow("Free speed [km/h]",vf_);f->addRow("Wave speed [km/h]",wave_);f->addRow("Jam density",jam_);f->addRow("Sensors [km]",sensors_);auto*init=new QPushButton("INITIALIZE / RESET");f->addRow(init);connect(init,&QPushButton::clicked,this,&MainWindow::initialize);tabs->addTab(g,"General");
 auto*e=new QWidget;auto*ev=new QVBoxLayout(e);on_=new QTableWidget(0,3);on_->setHorizontalHeaderLabels({"km","low","peak"});off_=new QTableWidget(0,2);off_->setHorizontalHeaderLabels({"km","exit fraction"});bn_=new QTableWidget(0,2);bn_->setHorizontalHeaderLabels({"km","capacity factor"});row(on_,{8,200,900});row(off_,{15,.15});row(bn_,{12,.75});
 ev->addWidget(new QLabel("On-ramps"));ev->addWidget(on_);auto*ao=new QPushButton("Add on-ramp");ev->addWidget(ao);ev->addWidget(new QLabel("Off-ramps"));ev->addWidget(off_);auto*af=new QPushButton("Add off-ramp");ev->addWidget(af);ev->addWidget(new QLabel("Bottlenecks"));ev->addWidget(bn_);auto*ab=new QPushButton("Add bottleneck");ev->addWidget(ab);connect(ao,&QPushButton::clicked,[=]{row(on_,{5,200,600});});connect(af,&QPushButton::clicked,[=]{row(off_,{10,.1});});connect(ab,&QPushButton::clicked,[=]{row(bn_,{12,.8});});tabs->addTab(e,"Elements");
 auto*d=new QWidget;auto*dv=new QVBoxLayout(d);demand_=new QTableWidget(0,2);demand_->setHorizontalHeaderLabels({"Time h","Demand veh/h"});for(auto p:SimulationConfig{}.demandProfile)row(demand_,{p.timeHour,p.demand});dv->addWidget(demand_);auto*ad=new QPushButton("Add demand point");dv->addWidget(ad);connect(ad,&QPushButton::clicked,[=]{row(demand_,{duration_->value(),3000});});tabs->addTab(d,"Demand");
 auto*i=new QWidget;auto*iv=new QVBoxLayout(i);inc_=new QTableWidget(0,5);inc_->setHorizontalHeaderLabels({"km","lane 0=all","start h","duration min","factor"});row(inc_,{10,2,1.25,20,0});iv->addWidget(inc_);auto*ai=new QPushButton("Add incident");iv->addWidget(ai);connect(ai,&QPushButton::clicked,[=]{row(inc_,{10,1,1,15,0});});tabs->addTab(i,"Incidents");

 auto*right=new QVBoxLayout;root->addLayout(right,1);auto*stat=new QGroupBox("Simulation Progress");auto*sg=new QGridLayout(stat);clock_=new QLabel;stepText_=new QLabel;status_=new QLabel("READY");active_=new QLabel("None");progress_=new QProgressBar;progress_->setRange(0,1000);sg->addWidget(new QLabel("Time:"),0,0);sg->addWidget(clock_,0,1);sg->addWidget(new QLabel("Step:"),0,2);sg->addWidget(stepText_,0,3);sg->addWidget(new QLabel("Status:"),0,4);sg->addWidget(status_,0,5);sg->addWidget(progress_,1,0,1,6);sg->addWidget(new QLabel("Active incidents:"),2,0);sg->addWidget(active_,2,1,1,5);right->addWidget(stat);
 auto*grid=new QGridLayout;right->addLayout(grid,1);auto*rg=new QGroupBox("Live Freeway");auto*rgl=new QVBoxLayout(rg);road_=new FreewayView;rgl->addWidget(road_);grid->addWidget(rg,0,0,1,2);grid->addWidget(plotGroup("Density",rho_),0,2);grid->addWidget(plotGroup("Speed",speed_),0,3);grid->addWidget(plotGroup("Flow",flow_),1,0);grid->addWidget(plotGroup("Sensor Speeds",sensorPlot_),1,1);grid->addWidget(plotGroup("Fundamental Diagram",fd_),1,2);grid->addWidget(plotGroup("Demand",demandPlot_),1,3);
 auto*ctrl=new QGroupBox("Simulation Control");auto*ch=new QHBoxLayout(ctrl);play_=new QPushButton("PLAY");pause_=new QPushButton("PAUSE");stop_=new QPushButton("STOP");step_=new QPushButton("STEP");leap_=new QPushButton("LEAP");leapBox_=new QComboBox;for(int m:{1,5,10,15,30,60})leapBox_->addItem(QString("%1 min").arg(m),m);speedBox_=new QComboBox;for(int x:{1,5,10,25,50,100})speedBox_->addItem(QString("%1x").arg(x),x);speedBox_->addItem("Maximum",0);speedBox_->setCurrentText("10x");for(auto*b:{play_,pause_,stop_,step_})ch->addWidget(b);ch->addWidget(new QLabel("Leap:"));ch->addWidget(leapBox_);ch->addWidget(leap_);ch->addWidget(new QLabel("Playback:"));ch->addWidget(speedBox_);ch->addStretch();right->addWidget(ctrl);
 connect(play_,&QPushButton::clicked,this,&MainWindow::play);connect(pause_,&QPushButton::clicked,this,&MainWindow::pause);connect(stop_,&QPushButton::clicked,this,&MainWindow::stop);connect(step_,&QPushButton::clicked,this,&MainWindow::step);connect(leap_,&QPushButton::clicked,this,&MainWindow::leap);
}
SimulationConfig MainWindow::readConfig()const{
 SimulationConfig c;c.freewayLength=L_->value();c.cellLength=dx_->value();c.nLanes=lanes_->value();c.simTime=duration_->value()*3600;c.dt=dt_->value();c.vFree=vf_->value();c.w=wave_->value();c.rhoJam=jam_->value();
 c.demandProfile.clear();for(int r=0;r<demand_->rowCount();r++)c.demandProfile.push_back({tv(demand_,r,0),tv(demand_,r,1)});
 c.onRamps.clear();for(int r=0;r<on_->rowCount();r++)c.onRamps.push_back({tv(on_,r,0),tv(on_,r,1),tv(on_,r,2)});
 c.offRamps.clear();for(int r=0;r<off_->rowCount();r++)c.offRamps.push_back({tv(off_,r,0),tv(off_,r,1)});
 c.bottlenecks.clear();for(int r=0;r<bn_->rowCount();r++)c.bottlenecks.push_back({tv(bn_,r,0),tv(bn_,r,1)});
 c.incidents.clear();for(int r=0;r<inc_->rowCount();r++)c.incidents.push_back({tv(inc_,r,0),(int)std::lround(tv(inc_,r,1)),tv(inc_,r,2),tv(inc_,r,3),tv(inc_,r,4)});
 c.detectorKm.clear();for(auto&s:sensors_->text().split(',',Qt::SkipEmptyParts)){bool ok=false;double x=s.trimmed().toDouble(&ok);if(!ok)throw std::runtime_error("Invalid sensor list.");c.detectorKm.push_back(x);}
 c.finalize();return c;
}
void MainWindow::setupEmptyPlots(){
 const auto&c=engine_.config();
 for(auto*p:{rho_,speed_,flow_}){p->clearPlottables();p->clearItems();p->xAxis->setRange(0,c.simTime/3600.);p->yAxis->setRange(0,c.freewayLength);p->xAxis->setLabel("Time [h]");p->yAxis->setLabel("Position [km]");}
 sensorPlot_->clearGraphs();sensorPlot_->clearItems();sensorPlot_->xAxis->setRange(0,c.simTime/3600.);sensorPlot_->yAxis->setRange(0,c.vFree*1.1);
 fd_->clearGraphs();fd_->clearItems();fd_->xAxis->setRange(0,c.rhoJam);fd_->yAxis->setRange(0,c.capacity*1.1);
 demandPlot_->clearGraphs();demandPlot_->clearItems();demandPlot_->xAxis->setRange(0,c.simTime/3600.);
}
void MainWindow::initialize(){
 try{timer_->stop();engine_.setConfig(readConfig());initialized_=true;simulationAccumulator_=0;lastPlotStep_=-1;plotEverySteps_=std::max(1,(int)std::lround(60.0/engine_.config().dt));status_->setText("PAUSED");setupEmptyPlots();refreshLightweight();}
 catch(const std::exception&e){QMessageBox::critical(this,"Configuration",e.what());}
}
void MainWindow::play(){
 if(!initialized_)initialize();if(!initialized_||engine_.state().finished)return;
 simulationAccumulator_=0;status_->setText("RUNNING");timer_->start(std::chrono::milliseconds(20));
}
void MainWindow::pause(){timer_->stop();simulationAccumulator_=0;status_->setText("PAUSED");refreshLightweight();if(engine_.state().currentStep>=1)refreshPlots();}
void MainWindow::stop(){timer_->stop();simulationAccumulator_=0;engine_.reset();status_->setText("STOPPED");setupEmptyPlots();refreshLightweight();}
void MainWindow::step(){if(!initialized_)initialize();if(!initialized_)return;timer_->stop();engine_.step();status_->setText(engine_.state().finished?"COMPLETE":"PAUSED");refreshLightweight();if(engine_.state().currentStep>=1)refreshPlots();}
void MainWindow::leap(){if(!initialized_)initialize();if(!initialized_)return;timer_->stop();status_->setText("LEAPING");QApplication::processEvents();engine_.advanceSeconds(leapBox_->currentData().toInt()*60.0);status_->setText(engine_.state().finished?"COMPLETE":"PAUSED");refreshLightweight();if(engine_.state().currentStep>=1)refreshPlots();}
void MainWindow::tick(){
 if(!initialized_||engine_.state().finished){timer_->stop();return;}
 constexpr double guiPeriod=.020;int playback=speedBox_->currentData().toInt(),steps=0;
 if(playback==0)steps=200;else{simulationAccumulator_+=playback*guiPeriod;steps=(int)(simulationAccumulator_/engine_.config().dt);if(steps>0)simulationAccumulator_-=steps*engine_.config().dt;}
 steps=std::clamp(steps,0,1000);for(int i=0;i<steps&&!engine_.state().finished;i++)engine_.step();
 refreshLightweight();
 if(steps>0&&(engine_.state().currentStep-lastPlotStep_>=plotEverySteps_||engine_.state().finished))refreshPlots();
 if(engine_.state().finished){timer_->stop();status_->setText("COMPLETE");if(engine_.state().currentStep>=1)refreshPlots();}
}
QString MainWindow::timeText(double s){int z=std::max(0,(int)std::lround(s));return QString("%1:%2:%3").arg(z/3600,2,10,QChar('0')).arg((z%3600)/60,2,10,QChar('0')).arg(z%60,2,10,QChar('0'));}
void MainWindow::refreshLightweight(){
 const auto&c=engine_.config();const auto&s=engine_.state();double now=s.time[s.currentStep],pct=100*now/c.simTime;
 clock_->setText(timeText(now)+" / "+timeText(c.simTime));stepText_->setText(QString("%1 / %2").arg(s.currentStep+1).arg(c.nSteps));progress_->setValue((int)std::lround(std::clamp(pct,0.,100.)*10));
 QStringList a;auto act=engine_.activeIncidents(now);for(size_t j=0;j<act.size();j++)if(act[j])a<<QString("I%1 @ %2 km, lane %3").arg(j+1).arg(c.incidents[j].locationKm).arg(c.incidents[j].lane==0?QString("ALL"):QString::number(c.incidents[j].lane));active_->setText(a.isEmpty()?"None":a.join(" | "));road_->setModel(&c,&s);
}
void MainWindow::heatmap(QCustomPlot*p,const std::vector<std::vector<double>>&m,const QString&label){
 const auto&c=engine_.config();const auto&s=engine_.state();int K=std::clamp(s.currentStep+1,2,c.nSteps),N=c.nCells;
 p->clearPlottables();p->clearItems();auto*map=new QCPColorMap(p->xAxis,p->yAxis);map->data()->setSize(K,N);
 double tEnd=std::max(c.dt/3600.,s.time[K-1]/3600.);map->data()->setRange(QCPRange(0,tEnd),QCPRange(0,c.freewayLength));
 double lo=std::numeric_limits<double>::infinity(),hi=-std::numeric_limits<double>::infinity();
 for(int k=0;k<K;k++)for(int i=0;i<N;i++){double z=m[k][i];if(std::isfinite(z)){map->data()->setCell(k,i,z);lo=std::min(lo,z);hi=std::max(hi,z);}}
 if(!std::isfinite(lo)||!std::isfinite(hi)){lo=0;hi=1;}if(std::abs(hi-lo)<1e-12){double d=std::max(1.,std::abs(lo)*.01);lo-=d;hi+=d;}map->setDataRange(QCPRange(lo,hi));
 p->xAxis->setRange(0,c.simTime/3600.);p->yAxis->setRange(0,c.freewayLength);p->xAxis->setLabel("Time [h]");p->yAxis->setLabel(label);p->replot();
}
void MainWindow::refreshPlots(){
 if(!initialized_||engine_.state().currentStep<1)return;const auto&c=engine_.config();const auto&s=engine_.state();int K=s.currentStep+1;
 heatmap(rho_,s.density,"Position [km]");heatmap(speed_,s.speed,"Position [km]");heatmap(flow_,s.flow,"Position [km]");
 sensorPlot_->clearGraphs();sensorPlot_->clearItems();QVector<double>x(K);for(int k=0;k<K;k++)x[k]=s.time[k]/3600.;for(size_t d=0;d<c.detectorCells.size();d++){QVector<double>y(K);for(int k=0;k<K;k++)y[k]=s.speed[k][c.detectorCells[d]];auto*g=sensorPlot_->addGraph();g->setData(x,y);g->setName(QString("S%1").arg(d+1));}sensorPlot_->legend->setVisible(true);sensorPlot_->xAxis->setRange(0,c.simTime/3600.);sensorPlot_->yAxis->setRange(0,c.vFree*1.1);sensorPlot_->replot();
 fd_->clearGraphs();fd_->clearItems();QVector<double>rx,qx;for(int k=0;k<K;k++)for(int i=0;i<c.nCells;i++){rx<<s.density[k][i];qx<<s.flow[k][i];}auto*fg=fd_->addGraph();fg->setLineStyle(QCPGraph::lsNone);fg->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc,3));fg->setData(rx,qx);fd_->xAxis->setRange(0,c.rhoJam);fd_->yAxis->setRange(0,c.capacity*1.1);fd_->replot();
 demandPlot_->clearGraphs();demandPlot_->clearItems();QVector<double>tx(401),dy(401);for(int i=0;i<401;i++){tx[i]=c.simTime/3600.*i/400.;dy[i]=engine_.demand(tx[i]*3600);}auto*dg=demandPlot_->addGraph();dg->setData(tx,dy);demandPlot_->xAxis->setRange(0,c.simTime/3600.);demandPlot_->yAxis->rescale();demandPlot_->replot();lastPlotStep_=s.currentStep;
}
