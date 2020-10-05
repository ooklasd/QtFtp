#pragma once

#include "export.h"
#include <QObject>
#include <QHostAddress>
#include "QTcpSocket"
#include "QTcpServer"
#include <array>
#include "QDateTime"
//#include "QFTP\qurlinfo.h"

class QNetworkAccessManager;
struct QUrlInfo
{
	QString name;
	bool isDir = false;
	int access = 0;
	size_t size = 0;
	QDateTime time;
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

	//�Ƿ��ѵ�¼����socket���Ӽ���¼
	bool isLogin()const{ return !!getConnectSocket(); }

	//��ȡ��ǰ�������ӵ�sockeet���Ͽ����Ӽ��޵�¼
	QTcpSocket* getConnectSocket()const{ return m_socket.data(); }

	//У����һ�������Ƿ񷵻���ȷ
	bool isLastVaild(char expect) const{
		return m_response.size() >= 3 ? (m_response.at(0) == expect) : false;
	}

	//��ȡ��һ������ķ�����
	QString getLastCode()const{ return toString(m_response.mid(0, 3)); }

	bool login(QString username, QString password);
	bool reset(size_t offset);

	bool CWD(QString path = "/");
	bool CDUP(){ return sendCMD("CDUP", '3'); }
	bool MKD(QString path){ return sendCMD("MKD "+path, '3'); }
	QString PWD();

	bool DELE(QString file){ return sendCMD("DELE " + file, '3'); }
	bool QUIT(){ return sendCMD("QUIT", '3'); }
	bool REIN(){ return sendCMD("REIN", '3'); }
	QList<QUrlInfo> LIST(QString path);
	QList<QUrlInfo> NLST(QString path);
	size_t SIZE(QString file);

	bool ABOR();
	bool NOOP(){ return sendCMD("NOOP", 2); }

public:
	//=======================================
	// ���Ե��õĵײ㹦��
	//=======================================

	//��ȡ��Ҫ�����socket��Ȼ���Լ�����io
	QTcpSocket* FtpAccess(QString cmd, transfermode mode);


protected:
	bool sendCMD(QString buffer,char expect);
	bool writeCMD(QString buffer);
	bool readresp(char expect);

	QTcpSocket* FtpOpenPasv(QString cmd, transfermode mode);
	QTcpSocket* FtpOpenPort(QString cmd, transfermode mode);

	size_t FtpAccessRead(QString cmd, transfermode mode, QIODevice* iofile);
	size_t FtpAccessWrite(QString cmd, transfermode mode, QIODevice* iofile);

	//��ȡ��ǰ��Ftp����
	void updateFtpCode();
	QString toString(const QByteArray& content)const;
	QByteArray toByte(const QString& content)const;

protected:
	QString m_username;
	QString m_password;
	short m_port;
	QHostAddress m_address;
	QByteArray m_response;
	QTextCodec* m_code = nullptr;
	size_t m_offset = 0;
	connmode m_connmode = pasv;
	QScopedPointer<QTcpSocket> m_socket;	//��������ִ��
};
