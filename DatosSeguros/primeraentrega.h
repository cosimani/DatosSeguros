#ifndef PRIMERAENTREGA_H
#define PRIMERAENTREGA_H

#include <QWidget>
#include <QResizeEvent>

namespace Ui {
class PrimeraEntrega;
}

class PrimeraEntrega : public QWidget
{
    Q_OBJECT

public:
    explicit PrimeraEntrega(QWidget *parent = 0);
    ~PrimeraEntrega();

private:
    Ui::PrimeraEntrega *ui;

protected:
    void resizeEvent( QResizeEvent * e );

private slots:
    void slot_procesar();
    void slot_elegirFoto();
};

#endif // PRIMERAENTREGA_H
