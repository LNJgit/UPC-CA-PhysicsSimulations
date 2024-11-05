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

    connect(ui->paintDensity, &QCheckBox::clicked, this, [this]() {
        ui->paintPressure->setChecked(false);
    });

    connect(ui->paintPressure, &QCheckBox::clicked, this, [this]() {
        ui->paintDensity->setChecked(false);
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


double WidgetSPH::getParticleMass() const {
    return ui->particleMass->value();
}

bool WidgetSPH::getLeftWallValue() const {
    return ui->leftWall->isChecked();
}

bool WidgetSPH::getRightWallValue() const {
    return ui->rightWall->isChecked();
}

bool WidgetSPH::getFrontWallValue() const {
    return ui->frontWall->isChecked();
}

bool WidgetSPH::getBackWallValue() const {
    return ui->backWall->isChecked();
}

double WidgetSPH::getWidthValue() const {
    return ui->cubeWidth->value();
}

double WidgetSPH::getHeightValue() const {
    return ui->cubeHeight->value();
}

int WidgetSPH::getNumInitialParticles() const {
    return ui->numInitialParticles->value();
}

bool WidgetSPH::getPaintMode() {
    if(ui->paintDensity->isChecked())
    {
        return true;
    }
    else
        {
            return false;
        }
}

double WidgetSPH::getCs() const
{
    return ui->cs->value();
}

