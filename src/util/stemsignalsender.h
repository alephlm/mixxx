#pragma once

#include <QObject>
#include <QThread>
#include <QUdpSocket>

class StemSignalSenderWorker : public QObject {
    Q_OBJECT
  public:
    StemSignalSenderWorker();
    ~StemSignalSenderWorker() override;

  public slots:
    void init(const QString& host, quint16 port);
    void send(int deckId, int stemId, int value);

  private:
    QUdpSocket* m_pSocket = nullptr;
    QString m_host;
    quint16 m_port = 0;
};

class StemSignalSender : public QObject {
    Q_OBJECT
  public:
    static void init(const QString& host = QStringLiteral("192.168.4.1"),
            quint16 port = 8888);
    static void send(int deckId, int stemId, int value);
    static void stop();

  private:
    static StemSignalSenderWorker* s_pWorker;
    static QThread* s_pThread;
};
