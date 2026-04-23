#pragma once

#include <QMutex>
#include <QObject>
#include <QSerialPort>
#include <QThread>

class LedSerialWorker : public QObject {
    Q_OBJECT
  public:
    LedSerialWorker();
    ~LedSerialWorker() override;

  public slots:
    void init(const QString& portName);
    void send(int deckId, int stemId, int value);

  private:
    QSerialPort m_port;
};

class LedSerial : public QObject {
    Q_OBJECT
  public:
    static void init(const QString& portName);
    static void send(int deckId, int stemId, int value);
    static void stop();

  private:
    static LedSerialWorker* s_pWorker;
    static QThread* s_pThread;
};
