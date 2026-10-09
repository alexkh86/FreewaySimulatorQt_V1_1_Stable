#include "FreewayView.h"
#include <QPainter>
FreewayView::FreewayView(QWidget*p):QWidget(p){setMinimumHeight(150);}
void FreewayView::setModel(const SimulationConfig*c,const SimulationState*s){c_=c;s_=s;update();}
void FreewayView::paintEvent(QPaintEvent*){
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);if(!c_)return;
    QRectF r(24,35,width()-48,height()-60);p.fillRect(r,QColor(55,55,58));double lh=r.height()/c_->nLanes;
    p.setPen(QPen(Qt::white,1,Qt::DashLine));for(int l=1;l<c_->nLanes;l++)p.drawLine(QPointF(r.left(),r.top()+l*lh),QPointF(r.right(),r.top()+l*lh));
    auto X=[&](double x){return r.left()+r.width()*x/c_->freewayLength;};
    if(s_){auto&v=s_->speed[s_->currentStep];for(int i=0;i<c_->nCells;i++){double z=v[i]/c_->vFree;QColor col=z<.4?QColor(220,50,50):z<.7?QColor(240,170,30):QColor(40,180,70);col.setAlpha(90);p.fillRect(QRectF(X(i*c_->cellLength),r.top(),X((i+1)*c_->cellLength)-X(i*c_->cellLength),r.height()),col);}}
    p.setPen(Qt::cyan);for(size_t i=0;i<c_->detectorKm.size();i++){double x=X(c_->detectorKm[i]);p.drawLine(QPointF(x,r.top()),QPointF(x,r.bottom()));p.drawText(QPointF(x-10,r.top()-7),QString("S%1").arg(i+1));}
    p.setPen(QPen(QColor(80,180,255),3));for(auto&a:c_->onRamps)p.drawLine(QPointF(X(a.locationKm)-45,r.bottom()+20),QPointF(X(a.locationKm),r.bottom()));
    p.setPen(QPen(QColor(190,120,255),3));for(auto&a:c_->offRamps)p.drawLine(QPointF(X(a.locationKm),r.bottom()),QPointF(X(a.locationKm)+45,r.bottom()+20));
    p.setPen(Qt::NoPen);p.setBrush(QColor(255,130,30,110));for(auto&b:c_->bottlenecks)p.drawRect(QRectF(X(b.locationKm)-4,r.top(),8,r.height()));
    double h=s_?s_->time[s_->currentStep]/3600.:-1;
    for(size_t j=0;j<c_->incidents.size();j++){auto&i=c_->incidents[j];bool active=h>=i.startHour&&h<i.endHour;p.setPen(QPen(active?Qt::red:Qt::darkRed,active?4:2));double y=i.lane==0?r.center().y():r.top()+(i.lane-.5)*lh,x=X(i.locationKm);p.drawLine(QPointF(x-7,y-7),QPointF(x+7,y+7));p.drawLine(QPointF(x-7,y+7),QPointF(x+7,y-7));}
}
