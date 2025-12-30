#include "util/ledserial.h"

#include <QDebug>
#include <QSerialPort>
#include <QSerialPortInfo>

static QSerialPort serial;

void LedSerial::init(const QString& serialIdPath) {
    if (serial.isOpen())
        return;

    serial.setPortName(serialIdPath);
    serial.setBaudRate(QSerialPort::Baud115200);

    if (!serial.open(QIODevice::ReadWrite)) {
        qWarning() << "Failed to open serial port"
                   << serialIdPath << ":" << serial.errorString();
    } else {
        qInfo() << "Serial port opened:" << serialIdPath;
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
