#include "util/ledserial.h"

#include <QDebug>
#include <QSerialPort>
#include <QSerialPortInfo>

static QSerialPort serial;

void LedSerial::init(const QString& portName) {
    if (serial.isOpen())
        return;

    serial.setPortName(portName);
    serial.setBaudRate(QSerialPort::Baud115200);

    if (!serial.open(QIODevice::WriteOnly)) {
        qWarning() << "((((((((((((((((((((((Failed to open serial port"
                   << portName << ":" << serial.errorString();
    } else {
        qInfo() << "))))))))))))))))))))))))))))))))))Serial port opened:" << portName;
    }
}

void LedSerial::send(uint8_t deckId, uint8_t channelId, uint8_t ledValue) {
    if (!serial.isOpen()) {
        qWarning() << "Serial port not open!";
        return;
    }

    QByteArray payload;
    payload.append((char)0xAA); // start marker
    payload.append((char)deckId);
    payload.append((char)channelId);
    payload.append((char)ledValue);

    qint64 bytesWritten = serial.write(payload);
    serial.flush();

    qInfo() << "Sent frame:"
            << "deck=" << deckId
            << "channel=" << channelId
            << "value=" << ledValue
            << "(wrote" << bytesWritten << "bytes)";
}
