#pragma once
#include <QMainWindow>
#include "Simulation.h"
class QTimer; class QDoubleSpinBox; class QSpinBox; class QLineEdit; class QTableWidget;
class QLabel; class QProgressBar; class QPushButton; class QComboBox; class QCustomPlot; class FreewayView;

class MainWindow:public QMainWindow{
 Q_OBJECT
public: MainWindow();
private slots: void initialize(); void play(); void pause(); void stop(); void step(); void leap(); void tick();
private:
 void buildUi(); SimulationConfig readConfig() const; void setupEmptyPlots();
 void refreshLightweight(); void refreshPlots();
 void heatmap(QCustomPlot*,const std::vector<std::vector<double>>&,const QString&);
 static QString timeText(double);
 CTMEngine engine_; QTimer* timer_=nullptr; bool initialized_=false;
 double simulationAccumulator_=0.0; int plotEverySteps_=12,lastPlotStep_=-1;
 QDoubleSpinBox *L_=nullptr,*dx_=nullptr,*duration_=nullptr,*dt_=nullptr,*vf_=nullptr,*wave_=nullptr,*jam_=nullptr;
 QSpinBox*lanes_=nullptr; QLineEdit*sensors_=nullptr; QTableWidget *demand_=nullptr,*on_=nullptr,*off_=nullptr,*bn_=nullptr,*inc_=nullptr;
 QLabel *clock_=nullptr,*stepText_=nullptr,*status_=nullptr,*active_=nullptr; QProgressBar*progress_=nullptr;
 QPushButton *play_=nullptr,*pause_=nullptr,*stop_=nullptr,*step_=nullptr,*leap_=nullptr; QComboBox *leapBox_=nullptr,*speedBox_=nullptr;
 FreewayView*road_=nullptr; QCustomPlot *rho_=nullptr,*speed_=nullptr,*flow_=nullptr,*sensorPlot_=nullptr,*fd_=nullptr,*demandPlot_=nullptr;
};
