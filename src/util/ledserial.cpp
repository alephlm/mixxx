#include "util/ledserial.h"

#include <QDebug>
#include <QMetaObject>

LedSerialWorker* LedSerial::s_pWorker = nullptr;
QThread* LedSerial::s_pThread = nullptr;

LedSerialWorker::LedSerialWorker() {
}

LedSerialWorker::~LedSerialWorker() {
    if (m_port.isOpen()) {
        m_port.close();
    }
}

void LedSerialWorker::init(const QString& portName) {
    if (m_port.isOpen() && m_port.portName() == portName) {
        return;
    }
    if (m_port.isOpen()) {
        m_port.close();
    }

    // Connect error signal if not already connected
    disconnect(&m_port, &QSerialPort::errorOccurred, nullptr, nullptr);
    connect(&m_port, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        if (error == QSerialPort::ResourceError || error == QSerialPort::PermissionError) {
            qWarning() << "LedSerialWorker: Fatal error, closing port:" << m_port.errorString();
            m_port.close();
        }
    });

    m_port.setPortName(portName);
    m_port.setBaudRate(QSerialPort::Baud115200);
    if (!m_port.open(QIODevice::WriteOnly)) {
        qWarning() << "LedSerialWorker: Failed to open" << portName << m_port.errorString();
    } else {
        qInfo() << "LedSerialWorker: Opened" << portName;
    }
}

void LedSerialWorker::send(int deckId, int stemId, int value) {
    if (!m_port.isOpen() || m_port.error() != QSerialPort::NoError)
        return;

    QByteArray payload;
    payload.append((char)0xAA);
    payload.append((char)static_cast<uint8_t>(deckId));
    payload.append((char)static_cast<uint8_t>(stemId));
    payload.append((char)static_cast<uint8_t>(value));

    if (m_port.write(payload) == -1) {
        qWarning() << "LedSerialWorker: Write failed, error:" << m_port.error();
    }
}

void LedSerial::init(const QString& portName) {
    if (!s_pThread) {
        s_pThread = new QThread();
        s_pThread->setObjectName("LedSerialThread");
        s_pWorker = new LedSerialWorker();
        s_pWorker->moveToThread(s_pThread);

        // Ensure cleanup
        connect(s_pThread, &QThread::finished, s_pWorker, &QObject::deleteLater);

        s_pThread->start();
    }

    QMetaObject::invokeMethod(s_pWorker, "init", Qt::QueuedConnection, Q_ARG(QString, portName));
}

void LedSerial::send(int deckId, int stemId, int value) {
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

void LedSerial::stop() {
    if (s_pThread) {
        s_pThread->quit();
        s_pThread->wait();
        delete s_pThread;
        s_pThread = nullptr;
        s_pWorker = nullptr;
    }
}
#include "moc_ledserial.cpp"
