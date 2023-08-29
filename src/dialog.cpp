#include "dialog.h"
#include "ui_dialog.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <cstdlib>

#define MAXTIMINGS 85
#define DHTPIN 2

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    QPixmap airTemp("/home/stefanrb3/Downloads/hot.png");
    QPixmap waterTemp("/home/stefanrb3/Downloads/water-temperature.png");
    QPixmap pressure("/home/stefanrb3/Downloads/pressure.png");
    QPixmap humidity("/home/stefanrb3/Downloads/humidity.png");

    ui->label_airtemp->setPixmap(airTemp);
    ui->label_airtemp->setScaledContents(true);
    ui->label_watertemp_3->setPixmap(waterTemp);
    ui->label_watertemp_3->setScaledContents(true);
    ui->label_humidity_2->setPixmap(humidity);
    ui->label_humidity_2->setScaledContents(true);
    ui->label_pressure_4->setPixmap(pressure);
    ui->label_pressure_4->setScaledContents(true);

    // Initialize wiringPi and SPI
    if (wiringPiSetup() == -1)
        exit(1);

    int fd;
    unsigned char buffer[6];

    if ((fd = wiringPiSPISetup(CHANNEL, SPEED)) < 0) {
        printf("SPI Setup failed.\n");
        exit(-1);
    }

    // Initialize the timer for reading sensor data
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(readSensorsData()));
    timer->start(1000);  // Read data every second
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::readSensorsData()
{
    // Read DHT11 sensor data
    readDHT11Data();

    // Read DS18B20 sensor data
    float ds18b20Temperature = readDS18B20Temperature();

    // Read BMP280 sensor data
    float bmp280Pressure = readBMP280Pressure();

    // Update labels with sensor data
    ui->label_temperature_value->setText(QString::number(dht11_dat[2]) + "." + QString::number(dht11_dat[3]) + "°C");
    ui->label_temperature2_value->setText(QString::number(ds18b20Temperature) + "°C");
    ui->label_humidity_value->setText(QString::number(dht11_dat[0]) + "." + QString::number(dht11_dat[1]) + "%");
    ui->label_pressure_value->setText(QString::number(bmp280Pressure) + " hPa");
}

void Dialog::readDHT11Data()
{
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;
    float f;

    dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;

    pinMode(DHTPIN, OUTPUT);
    digitalWrite(DHTPIN, LOW);
    delay(18);
    digitalWrite(DHTPIN, HIGH);
    delayMicroseconds(40);
    pinMode(DHTPIN, INPUT);

    for (i = 0; i < MAXTIMINGS; i++)
    {
        counter = 0;
        while (digitalRead(DHTPIN) == laststate)
        {
            counter++;
            delayMicroseconds(1);
            if (counter == 255)
            {
                break;
            }
        }
        laststate = digitalRead(DHTPIN);

        if (counter == 255)
            break;

        if ((i >= 4) && (i % 2 == 0))
        {
            dht11_dat[j / 8] <<= 1;
            if (counter > 16)
                dht11_dat[j / 8] |= 1;
            j++;
        }
    }

    if ((j >= 40) &&
        (dht11_dat[4] == ((dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF)))
    {
        f = dht11_dat[2] * 9. / 5. + 32;
        printf("DHT11 - Humidity = %d.%d %% Temperature = %d.%d C (%.1f F)\n",
               dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3], f);
    }
    else
    {
        printf("DHT11 - Data not good, skip\n");
    }
}

float Dialog::readDS18B20Temperature()
{
    int ds18b20_fd = -1, ds18b20_ret;
    char ds18b20_tmp2[10], ds18b20_ch = 't';
    char ds18b20_buffer[100];

   if ((ds18b20_fd = ::open(DS18B20_PATH, O_RDONLY)) < 0) {
        perror("Error opening DS18B20 device file");
        exit(1);
    }

    ds18b20_ret = read(ds18b20_fd, ds18b20_buffer, sizeof(ds18b20_buffer));

    if (ds18b20_ret < 0) {
        perror("Error reading from DS18B20 device file");
        exit(1);
    }

    char *ds18b20_tmp1 = strchr(ds18b20_buffer, ds18b20_ch);
    if (ds18b20_tmp1 == NULL) {
        fprintf(stderr, "DS18B20 - Temperature data not found in buffer.\n");
       ::close(ds18b20_fd);
        exit(1);
    }

    if (sscanf(ds18b20_tmp1, "t=%s", ds18b20_tmp2) != 1) {
        fprintf(stderr, "DS18B20 - Error parsing temperature data.\n");
        ::close(ds18b20_fd);
        exit(1);
    }

    long ds18b20_value = atoi(ds18b20_tmp2);
    int ds18b20_integer = ds18b20_value / 1000;
    int ds18b20_decimal = ds18b20_value % 1000;

    ::close(ds18b20_fd);

    return ds18b20_integer + ds18b20_decimal / 1000.0;
}

float Dialog::readBMP280Pressure()
{
    digitalWrite(CS_PIN, LOW);

    // Read pressure data from BMP280
    unsigned char buffer[3];
    buffer[0] = 0x34; // BMP280 pressure command
    wiringPiSPIDataRW(CHANNEL, buffer, 3);

    // Convert pressure data (assumes 20-bit resolution)
    int32_t adc_P = (buffer[0] << 12) | (buffer[1] << 4) | (buffer[2] >> 4);
    float pressure = ((float)adc_P / 1048576.0) * 100.0;

    // Deselect the BMP280 device (CS pin high)
    digitalWrite(CS_PIN, HIGH);

    return pressure;
}
