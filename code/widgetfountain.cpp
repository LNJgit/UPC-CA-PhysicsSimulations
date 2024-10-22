#include "widgetfountain.h"
#include "ui_widgetfountain.h"

WidgetFountain::WidgetFountain(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetFountain)
{
    ui->setupUi(this);

    connect(ui->btnUpdate, &QPushButton::clicked, this,
            [=] (void) { emit updatedParameters(); });
}

WidgetFountain::~WidgetFountain()
{
    delete ui;
}

double WidgetFountain::getGravity() const {
    return ui->gravity->value();
}

double WidgetFountain::getEmitRate() const {
    return ui->emitRate->value();
}

double WidgetFountain::getElasticity() const {
    return ui->elasticityCoefficient->value();
}

double WidgetFountain::getFriction() const {
    return ui->frictionCoeff->value();
}

double WidgetFountain::getParticleLife() const
{
    return ui->particleLife->value();
}



