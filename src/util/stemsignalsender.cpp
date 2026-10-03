#include "util/stemsignalsender.h"

#include <QDebug>
#include <QMetaObject>

StemSignalSenderWorker* StemSignalSender::s_pWorker = nullptr;
QThread* StemSignalSender::s_pThread = nullptr;

StemSignalSenderWorker::StemSignalSenderWorker() {
}

StemSignalSenderWorker::~StemSignalSenderWorker() {
    if (m_pSocket) {
        m_pSocket->close();
        delete m_pSocket;
        m_pSocket = nullptr;
    }
}

void StemSignalSenderWorker::init(const QString& host, quint16 port) {
    m_host = host;
    m_port = port;

    if (!m_pSocket) {
        m_pSocket = new QUdpSocket(this);
    }

    qInfo().noquote()
            << "StemSignalSenderWorker: UDP target set to"
            << m_host + ":" + QString::number(m_port);
}

void StemSignalSenderWorker::send(int deckId, int stemId, int value) {
    if (!m_pSocket || m_host.isEmpty() || m_port == 0)
        return;

    QByteArray payload;
    payload.append(static_cast<char>(0xAA));
    payload.append(static_cast<char>(static_cast<uint8_t>(deckId)));
    payload.append(static_cast<char>(static_cast<uint8_t>(stemId)));
    payload.append(static_cast<char>(static_cast<uint8_t>(value)));

    qint64 sent = m_pSocket->writeDatagram(
            payload, QHostAddress(m_host), m_port);
    if (sent == -1) {
        qWarning() << "StemSignalSenderWorker: UDP send failed:" << m_pSocket->errorString();
    }
}

void StemSignalSender::init(const QString& host, quint16 port) {
    if (!s_pThread) {
        s_pThread = new QThread();
        s_pThread->setObjectName("StemSignalSenderThread");
        s_pWorker = new StemSignalSenderWorker();
        s_pWorker->moveToThread(s_pThread);

        // Ensure cleanup
        connect(s_pThread, &QThread::finished, s_pWorker, &QObject::deleteLater);

        s_pThread->start();
    }

    QMetaObject::invokeMethod(s_pWorker,
            "init",
            Qt::QueuedConnection,
            Q_ARG(QString, host),
            Q_ARG(quint16, port));
}

void StemSignalSender::send(int deckId, int stemId, int value) {
    if (!s_pWorker)
        return;

    // This is non-blocking and thread-safe.
    QMetaObject::invokeMethod(s_pWorker,
            "send",
            Qt::QueuedConnection,
            Q_ARG(int, deckId),
            Q_ARG(int, stemId),
            Q_ARG(int, value));
}

void StemSignalSender::stop() {
    if (s_pThread) {
        s_pThread->quit();
        s_pThread->wait();
        delete s_pThread;
        s_pThread = nullptr;
        s_pWorker = nullptr;
    }
}
#include "moc_stemsignalsender.cpp"
