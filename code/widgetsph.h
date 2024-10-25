#ifndef WIDGETSPH_H
#define WIDGETSPH_H

#include <QMainWindow>
#include <QObject>
#include <QWidget>

namespace Ui {
class WidgetSPH;
}

class WidgetSPH : public QWidget
{
    Q_OBJECT
public:
    explicit WidgetSPH(QWidget *parent = nullptr);
    ~WidgetSPH();

    // Getter methods for the SPH parameters
    double getDensity() const;
    double getViscosity() const;
    double getSurfaceTension() const;
    double getPressureStiffness() const;
    double getParticleMass() const;

signals:
    void updatedParameters();

private:
    Ui::WidgetSPH *ui;
};

#endif // WIDGETSPH_H
