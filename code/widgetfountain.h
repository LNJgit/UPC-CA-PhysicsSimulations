#ifndef WIDGETFOUNTAIN_H
#define WIDGETFOUNTAIN_H

#include <QWidget>

namespace Ui {
class WidgetFountain;
}

class WidgetFountain : public QWidget
{
    Q_OBJECT
public:
    explicit WidgetFountain(QWidget *parent = nullptr);
    ~WidgetFountain();

    double getGravity()    const;
    double getEmitRate()   const;
    double getFriction()   const;
    double getElasticity() const;
    double getParticleLife() const;
    double getBlackHoleMass() const;
    double getBlackHoleRadius() const;
    bool   blackHoleEnabled() const;

signals:
    void updatedParameters();

private:
    Ui::WidgetFountain *ui;
};

#endif // WIDGETFOUNTAIN_H
