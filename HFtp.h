#pragma once

#include "export.h"
#include <QObject>
#include <QHostAddress>
#include "QTcpSocket"
#include "QTcpServer"
#include <array>
//#include "QFTP\qurlinfo.h"

class QNetworkAccessManager;
struct QUrlInfo
{
	QString name;
	bool isDir = false;
	int access = 0;
};
class QIODevice;
class HIQTWIDGET_EXPORT HFtp : public QObject
{
	Q_OBJECT
public:
	enum transfermode
	{
		ascii = 'A',
		image = 'I'
	};

	enum connmode
	{
		pasv = 1,
		port
	};

	HFtp(QObject *parent = nullptr);
	~HFtp();

	bool connectFTP(QString address, short port = 21);
	bool connectFTP(QHostAddress address, short port = 21);

	bool login(QString username, QString password);
	bool reset(size_t offset);

	QList<QUrlInfo> LIST(QString path);
	QList<QUrlInfo> NLST(QString path);

protected:
	bool sendCMD(QString buffer,char expect);
	bool readresp(char expect);

	QTcpSocket* FtpOpenPasv(QString cmd, transfermode mode);
	QTcpSocket* FtpOpenPort(QString cmd, transfermode mode);

	QTcpSocket* FtpAccess(QString cmd, transfermode mode);
	size_t FtpAccessRead(QString cmd, transfermode mode, QIODevice* iofile);
	size_t FtpAccessWrite(QString cmd, transfermode mode, QIODevice* iofile);

	QString toString(const QByteArray& content);
	QByteArray toByte(const QString& content);

protected:
	QString m_username;
	QString m_password;
	short m_port;
	QHostAddress m_address;
	QByteArray m_response;
	QTextCodec* m_code;
	size_t m_offset = 0;
	connmode m_connmode = pasv;
	QScopedPointer<QTcpSocket> m_socket;	//”√”⁄√¸¡Ó÷¥––
};
