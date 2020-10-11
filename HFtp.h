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
	enum TransferMode
	{
		ascii = 'A',
		image = 'I'
	};

	enum ConnectMode
	{
		pasv = 1,
		port
	};

	HFtp(QObject *parent = nullptr);
	~HFtp();

	bool connectFTP(QString address, short port = 21);
	bool connectFTP(QHostAddress address, short port = 21);

	//�Ƿ��ѵ�¼����socket���Ӽ���¼
	bool isLogin()const{ return _cmdSocket && _cmdSocket->isValid(); }

	//��ȡ��ǰ�������ӵ�sockeet���Ͽ����Ӽ��޵�¼�������ڰ��ź�
	QTcpSocket* getConnectSocket()const{ return _cmdSocket.data(); }

	//У����һ�������Ƿ񷵻���ȷ
	bool isLastVaild(char expect) const{
		return _response.size() >= 3 ? (_response.at(0) == expect) : false;
	}

	//��ȡ��һ������ķ�����
	QString getLastCode()const{ return toString(_response.mid(0, 3)); }

	bool login(QString username, QString password);
	void logout(){ return quit(); }
	bool reset(size_t offset);

	bool cwd(QString path = "/");
	bool cdup(){ return sendCMD("CDUP", '3'); }
	bool mkd(QString path){ return sendCMD("MKD "+path, '2'); }
	QString pwd();

	bool dele(QString file){ return sendCMD("DELE " + file, '3'); }
	void quit();
	bool rein(){ return sendCMD("REIN", '3'); }
	//��ȡ��ϸ�б�
	QList<QUrlInfo> list(QString path);
	//��ȡ�����б�
	QList<QUrlInfo> nlst(QString path);
	size_t size(QString file);
	
	size_t get(QString remoteFile, QIODevice& outDevice);
	size_t get(QString remoteFile, QString localFile);

	size_t put(QIODevice& inDevice, QString remoteFile);
	size_t put(QString localFile, QString remoteFile);

	bool abort();
	bool noop(){ return sendCMD("NOOP", 2); }

	HFtp::ConnectMode getConnectMode() const { return _connectMode; }
	void setConnectMode(HFtp::ConnectMode val) { _connectMode = val; }
public:
	//=======================================
	// ���Ե��õĵײ㹦��
	//=======================================

	//��ȡ��Ҫ�����socket��Ȼ���Լ�����io
	QTcpSocket* transferData(QString cmd, TransferMode mode,size_t offset = 0);

	bool writeCMD(QString buffer);
	bool readresp(char expect);

	//������������ս��������expect�����ж�
	bool sendCMD(QString buffer, char expect);

Q_SIGNALS:
	void onAbort(QPrivateSignal);

protected:
	//����������,������ֹ�źţ�����SSL�򷵻�SSL socket
	virtual QTcpSocket* netSocket();

	void bindSocket(QTcpSocket* socket);

	QTcpSocket* openPasv(QString cmd, TransferMode mode, size_t offset = 0);
	QTcpSocket* openPort(QString cmd, TransferMode mode, size_t offset = 0);

	size_t transferRead(QString cmd, TransferMode mode, QIODevice* iofile);
	size_t transferWrite(QString cmd, TransferMode mode, QIODevice* iofile);

	//��ȡ��ǰ��Ftp����
	void updateFtpCode();
	QString toString(const QByteArray& content)const;
	QByteArray toByte(const QString& content)const;

protected:
	QString _username;
	QString _password;
	short _port = 21;
	QHostAddress _address;
	QByteArray _response;
	QTextCodec* _code = nullptr;
	ConnectMode _connectMode = ConnectMode::pasv;
	QScopedPointer<QTcpSocket> _cmdSocket;	//��������ִ��
};
