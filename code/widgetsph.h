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
    double getParticleMass() const;
    double getCs()const;
    bool getLeftWallValue() const;
    bool getRightWallValue() const;
    bool getFrontWallValue() const;
    bool getBackWallValue() const;
    double getWidthValue() const;
    double getHeightValue() const;
    int getNumInitialParticles() const;
    bool getPaintMode();

signals:
    void updatedParameters();

private:
    Ui::WidgetSPH *ui;
};

#endif // WIDGETSPH_H
