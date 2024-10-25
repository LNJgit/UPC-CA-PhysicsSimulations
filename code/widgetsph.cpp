#include "widgetsph.h"
#include "ui_widgetSPH.h"

WidgetSPH::WidgetSPH(QWidget *parent)
    : QWidget(parent), ui(new Ui::WidgetSPH)
{
    ui->setupUi(this);

    // Connect the update button to emit updatedParameters signal
    connect(ui->btnUpdate, &QPushButton::clicked, this, [=] {
        emit updatedParameters();
    });
}

WidgetSPH::~WidgetSPH()
{
    delete ui;
}

// Example getter methods for parameters, assuming they are QDoubleSpinBox or similar widgets
double WidgetSPH::getDensity() const {
    return ui->density->value();
}

double WidgetSPH::getViscosity() const {
    return ui->viscosity->value();
}

double WidgetSPH::getSurfaceTension() const {
    return ui->surfaceTension->value();
}

double WidgetSPH::getPressureStiffness() const {
    return ui->pressureStiffness->value();
}

double WidgetSPH::getParticleMass() const {
    return ui->particleMass->value();
}
