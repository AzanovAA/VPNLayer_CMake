/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Solutions component.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextStream>
#include <QDateTime>
#include <QStringList>
#include <QDir>
#include <QSettings>
#include <QProcess>
#include <QThread>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSharedMemory>
#include <windows.h>
#include "log.h"

#include "src/qtservice.h"


class ServerDaemon : public QThread
{
    Q_OBJECT
public:
    ServerDaemon(QString appPath, QObject * parent = 0) : QThread(parent), hExitEvent_(NULL),
        appPath_(appPath), state_(Disconnected), socket_(NULL), bFinished_(false), sharedMemory("VPNLayedShared")
    {
        errorsWhileConnecting_ << "TLS Error: Need PEM pass phrase for private key" <<
            "EVP_DecryptFinal:bad decrypt" <<
            "PKCS12_parse:mac verify failure" <<
            "Received AUTH_FAILED control message" <<
            "Auth username is empty" <<
            "error=certificate has expired" <<
            "error=certificate is not yet valid";

        exitEventName_ = "vpnlayer_exit_event";
        if (sharedMemory.create(4096))
        {
            ALog::Out("Shared Memory Created");
        }
        else
        {
            ALog::Out("Shared Memory Error: " + sharedMemory.errorString());
        }
        //localServer_ = new QLocalServer(this);
        //localServer_->listen("\\\\.\\pipe\\vpnlayer");
        //connect(localServer_, SIGNAL(newConnection()), SLOT(socketNewConnection()));

        //connect(&process_, SIGNAL(readyRead()), SLOT(onOpenVPNReadyRead()));
        //connect(&process_, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(onOpenVPNFinished()));
        //start();
    }

    virtual ~ServerDaemon()
    {
        bFinished_ = true;
    }


protected:
    void run()
    {
        while (!bFinished_)
        {
            msleep(1);
            if (socket_ && socket_->isOpen())
            {
                if (socket_->bytesAvailable() >= sizeof(unsigned int))
                {
                    QDataStream stream(socket_);
                    unsigned int commandId;
                    stream >> commandId;
                    if (commandId == ConnectCommandId && state_ == Disconnected)
                    {
                        ALog::Out("ConnectCommand");
                        QString username, password, ovpnFile, configPath;
                        stream >> username;
                        stream >> password;
                        stream >> ovpnFile;
                        stream >> configPath;

                        hExitEvent_ = CreateEventW (NULL, TRUE, FALSE, (wchar_t *)exitEventName_.utf16());

                        userName_ = username;
                        password_ = password;

                        QStringList commandParams;
                        commandParams << "--service" << exitEventName_ << "0" << "--config" << ovpnFile;

                        QString openVPNExePath = appPath_ + "/openvpn.exe";

                        process_.setWorkingDirectory(configPath);
                        process_.setProcessChannelMode(QProcess::MergedChannels);
                        process_.start(openVPNExePath, commandParams, QIODevice::ReadWrite | QIODevice::Text);
                        state_ = Connecting;
                    }
                    else if (commandId == DisconnectCommandId && state_ != Disconnected)
                    {
                        ALog::Out("DisconnectCommand");
                        if (hExitEvent_)
                        {
                            SetEvent(hExitEvent_);
                            process_.blockSignals(true);
                            process_.waitForFinished();
                            process_.blockSignals(false);
                            CloseHandle(hExitEvent_);
                            hExitEvent_ = NULL;

                            QDataStream stream(socket_);
                            unsigned int commandDisconnected = 2;
                            stream << commandDisconnected;
                            stream << false;
                            state_ = Disconnected;
                        }
                    }
                }
            }
        }

    }

private slots:
    void socketNewConnection()
    {
        QLocalSocket *clientConnection = localServer_->nextPendingConnection();
        if (socket_ == NULL)
        {
            socket_ = clientConnection;
            clientConnection->connect(socket_, SIGNAL(disconnected()), SLOT(onDisconnected()));
        }
        else
        {
            clientConnection->close();
            clientConnection->deleteLater();
        }
        //connect(clientConnection, SIGNAL(readyRead()), SLOT(onReadyRead()));
        //clientConnection->connect(clientConnection, SIGNAL(disconnected()), SLOT(deleteLater()));
    }

    void onDisconnected()
    {
        socket_->deleteLater();
        socket_ = NULL;
    }

    /*void onReadyRead()
    {
        socket_ = dynamic_cast<QLocalSocket *>(sender());
        if (socket_)
        {
            QDataStream stream(socket_);
            unsigned int commandId;
            stream >> commandId;
            if (commandId == ConnectCommandId && state_ == Disconnected)
            {
                ALog::Out("ConnectCommand");
                QString username, password, ovpnFile, configPath;
                stream >> username;
                stream >> password;
                stream >> ovpnFile;
                stream >> configPath;

                hExitEvent_ = CreateEventW (NULL, TRUE, FALSE, (wchar_t *)exitEventName_.utf16());

                userName_ = username;
                password_ = password;

                QStringList commandParams;
                commandParams << "--service" << exitEventName_ << "0" << "--config" << ovpnFile;

                QString openVPNExePath = appPath_ + "/openvpn.exe";

                process_.setWorkingDirectory(configPath);
                process_.setProcessChannelMode(QProcess::MergedChannels);
                process_.start(openVPNExePath, commandParams, QIODevice::ReadWrite | QIODevice::Text);
                state_ = Connecting;
            }
            else if (commandId == DisconnectCommandId && state_ != Disconnected)
            {
                ALog::Out("DisconnectCommand");
                if (hExitEvent_)
                {
                    SetEvent(hExitEvent_);
                    process_.blockSignals(true);
                    process_.waitForFinished();
                    process_.blockSignals(false);
                    CloseHandle(hExitEvent_);
                    hExitEvent_ = NULL;

                    QDataStream stream(socket_);
                    unsigned int commandDisconnected = 2;
                    stream << commandDisconnected;
                    stream << false;
                    state_ = Disconnected;
                }
            }
        }
    }*/

    bool containsWhileConnectingError(QString line, QString &err)
    {
        foreach(QString s, errorsWhileConnecting_)
        {
            if (line.contains(s, Qt::CaseInsensitive))
            {
                return true;
            }
        }

        return false;
    }

    void sendLog(QString message)
    {
        if (socket_)
        {
            QDataStream stream(socket_);
            unsigned int commandLog = LogCommandId;
            stream << commandLog;
            stream << message;
        }
    }

    void onOpenVPNReadyRead()
    {
        QByteArray data = process_.readLine();
        QString err;
        while (!data.isEmpty())
        {
            QString line = data;
            if (line.contains("Enter Auth Username:", Qt::CaseInsensitive))
            {
                process_.write(userName_.toLocal8Bit());
            }
            else if (line.contains("Enter Auth Password:", Qt::CaseInsensitive))
            {
                process_.write(password_.toLocal8Bit());
            }
            else if (line.contains("Initialization Sequence Completed", Qt::CaseInsensitive))
            {
                state_ = Connected;
                {
                    QDataStream stream(socket_);
                    unsigned int commandConnected = ConnectCommandId;
                    stream << commandConnected;
                }
                sendLog(line.trimmed());
                //emit connected();
                ALog::Out(line.trimmed());
            }
            else if (containsWhileConnectingError(line, err))
            {
               // emit log(line);
                //emit error(err);
                ALog::Out(line.trimmed());
                {
                    QDataStream stream(socket_);
                    unsigned int commandError = ErrorCommandId;
                    stream << commandError;
                    stream << err;
                }
                sendLog(line.trimmed());
            }
            else if (state_ == Connected && line.contains("process restarting", Qt::CaseInsensitive))
            {
                if (hExitEvent_)
                {
                    SetEvent(hExitEvent_);
                    process_.blockSignals(true);
                    process_.waitForFinished();
                    process_.blockSignals(false);
                    CloseHandle(hExitEvent_);
                    hExitEvent_ = NULL;

                    QDataStream stream(socket_);
                    unsigned int commandDisconnected = DisconnectCommandId;
                    stream << commandDisconnected;
                    stream << true;
                    state_ = Disconnected;
                }

               // emit disconnected(true);
               // bConnected_ = false;
            }
            else
            {
               // emit log(line);
            }

            sendLog(line.trimmed());
            ALog::Out(line.trimmed());

            //qDebug() << data;
            data = process_.readLine();
        }
    }

    void onOpenVPNFinished()
    {
        if (state_ == Connected)
        {
            if (hExitEvent_)
            {
                CloseHandle(hExitEvent_);
                hExitEvent_ = NULL;
            }

            QDataStream stream(socket_);
            unsigned int commandDisconnected = DisconnectCommandId;
            stream << commandDisconnected;
            stream << true;

            //emit disconnected(true);
        }
        else
        {
            QDataStream stream(socket_);
            unsigned int commandDisconnected = DisconnectCommandId;
            stream << commandDisconnected;
            stream << true;
            //emit error("OpenVPN connect error 2");
            //emit disconnected(true);
        }
    }

private:
    enum { ConnectCommandId = 1, DisconnectCommandId = 2, LogCommandId = 3, ErrorCommandId = 4};
    enum { Connecting = 0, Connected = 1, Disconnected = 2 };
    QLocalServer *localServer_;
    QString exitEventName_;
    HANDLE hExitEvent_;
    QString userName_;
    QString password_;
    QProcess process_;
    QString appPath_;
    int state_;
    QStringList errorsWhileConnecting_;
    QLocalSocket *socket_;
    bool bFinished_;
    QSharedMemory sharedMemory;
};


class VPNService : public QtService<QCoreApplication>
{
public:
    VPNService(int argc, char **argv)
    : QtService<QCoreApplication>(argc, argv, "VPNLayer OpenVPN Daemon")
    {
        setServiceDescription("VPNLayer OpenVPN Daemon");
        //setServiceFlags(QtServiceBase::CanBeSuspended);
    }

protected:
    void start()
    {
        QCoreApplication *app = application();

        ALog::path_ = app->applicationDirPath();
        server = new ServerDaemon(app->applicationDirPath(), app);
    }

private:
    ServerDaemon *server;
    QFile file;
};

#include "main.moc"

int main(int argc, char **argv)
{
    VPNService service(argc, argv);
    return service.exec();
}
