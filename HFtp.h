#pragma once

#include "export.h"
#include <QObject>
#include <QHostAddress>
#include "QTcpSocket"
#include <array>
#include "QFTP\qurlinfo.h"

class QNetworkAccessManager;

class HIQTWIDGET_EXPORT HFtp : public QTcpSocket
{
	Q_OBJECT
public:
	enum transfermode
	{
		ascii = 'A',
		image = 'I'
	};

	enum accesstype
	{
		dir = 1,
		dirverbose,
		fileread,
		filewrite,
		filereadappend,
		filewriteappend
	};

	HFtp(QObject *parent = nullptr);
	~HFtp();

	bool connectFTP(QString address, short port = 21);
	bool connectFTP(QHostAddress address, short port = 21);

	QList<QUrlInfo> list(QString path);
	bool login(QString username, QString password);

protected:
	bool sendCMD(QString buffer,char expect);
	bool readresp(char expect);

	void FtpAccess(QIODevice* iofile, accesstype type, transfermode mode);


protected:
	QString m_username;
	QString m_password;
	short m_port;
	QHostAddress m_address;
	std::array<char, 3> m_response;

	QScopedPointer<QTcpSocket> m_socket;	//用于命令执行
	QScopedPointer<QNetworkAccessManager> m_networkManager;//用于下载ftp文件
};
