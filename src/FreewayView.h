#pragma once
#include <QWidget>
#include "Simulation.h"
class FreewayView:public QWidget{
    Q_OBJECT
public:
    explicit FreewayView(QWidget*parent=nullptr);
    void setModel(const SimulationConfig*c,const SimulationState*s);
protected:void paintEvent(QPaintEvent*)override;
private:const SimulationConfig*c_=nullptr;const SimulationState*s_=nullptr;
};
