#pragma once
#include <QMutex>
#include <QSerialPort>

class LedSerial {
  public:
    static void init(const QString& portName);
    static void send(uint8_t deckId, uint8_t stemId, uint8_t value);

  private:
    static QSerialPort* s_port;
    static QMutex s_mutex;
};
