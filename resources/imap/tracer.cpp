#include "tracer.h"
#include "imapresource_debug.h"

#include <QString>
#include <QIODevice>
#include <QCoreApplication>
#include <iostream>

class DebugStream: public QIODevice
{
public:
    QString m_location;
    DebugStream()
        : QIODevice()
    {
        open(WriteOnly);
    }
    virtual ~DebugStream() {}

    bool isSequential() const override
    {
        return true;
    }
    qint64 readData(char *, qint64) override {
        return 0; /* eof */
    }
    qint64 readLineData(char *, qint64) override {
        return 0; /* eof */
    }
    qint64 writeData(const char *data, qint64 len) override {
        const QByteArray buf = QByteArray::fromRawData(data, len);
        if (!qEnvironmentVariableIsEmpty("IMAP_TRACE"))
        {
            // qt_message_output(QtDebugMsg, buf.trimmed().constData());
            std::cout << buf.trimmed().constData() << std::endl;
        }
        return len;
    }
private:
    Q_DISABLE_COPY(DebugStream)
};

QDebug debugStream(int line, const char *file, const char *function)
{
    static DebugStream stream;
    QDebug debug(&stream);

    static QByteArray programName;
    if (programName.isEmpty()) {
        if (QCoreApplication::instance()) {
            programName = QCoreApplication::instance()->applicationName().toLocal8Bit();
        } else {
            programName = "<unknown program name>";
        }
    }

    Q_UNUSED(line);
    Q_UNUSED(file);
    debug << QStringLiteral("Trace:%1(%2) %3:").arg(QString::fromLatin1(programName)).arg(unsigned(QCoreApplication::applicationPid())).arg(QLatin1String(function)) /* << file << ":" << line */;

    return debug;
}
