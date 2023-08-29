#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QLabel>
#include <QTimer>
#include <wiringPi.h>
#include <wiringPiSPI.h>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();

private slots:
    void readSensorsData();

private:
    Ui::Dialog *ui;
    QTimer *timer;

    int dht11_dat[5] = {0, 0, 0, 0, 0};
    const char* DS18B20_PATH = "/sys/bus/w1/devices/28-3cb0f649967f/w1_slave";
    const int CHANNEL = 1;
    const int SPEED = 500000;
    const int MODE = 0;
    const int BITS = 8;
    const int CS_PIN = 7;


    void readDHT11Data();
    float readDS18B20Temperature();
    float readBMP280Pressure();
};

#endif // DIALOG_H
