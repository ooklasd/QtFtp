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

	//是否已登录，有socket连接即登录
	bool isLogin()const{ return _cmdSocket && _cmdSocket->isValid(); }

	//获取当前正在连接的sockeet，断开连接即无登录，可用于绑定信号
	QTcpSocket* getConnectSocket()const{ return _cmdSocket.data(); }

	//校验上一次命令是否返回正确
	bool isLastVaild(char expect) const{
		return _response.size() >= 3 ? (_response.at(0) == expect) : false;
	}

	//获取上一次命令的返回码
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
	//获取详细列表
	QList<QUrlInfo> list(QString path);
	//获取名字列表
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
	// 可以调用的底层功能
	//=======================================

	//获取需要传输的socket，然后自己处理io
	QTcpSocket* transferData(QString cmd, TransferMode mode,size_t offset = 0);

	bool writeCMD(QString buffer);
	bool readresp(char expect);

	//发送命令，并接收结果数字与expect进行判断
	bool sendCMD(QString buffer, char expect);

Q_SIGNALS:
	void onAbort(QPrivateSignal);

protected:
	//创建新连接,并绑定中止信号，若是SSL则返回SSL socket
	virtual QTcpSocket* netSocket();

	void bindSocket(QTcpSocket* socket);

	QTcpSocket* openPasv(QString cmd, TransferMode mode, size_t offset = 0);
	QTcpSocket* openPort(QString cmd, TransferMode mode, size_t offset = 0);

	size_t transferRead(QString cmd, TransferMode mode, QIODevice* iofile);
	size_t transferWrite(QString cmd, TransferMode mode, QIODevice* iofile);

	//获取当前的Ftp编码
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
	QScopedPointer<QTcpSocket> _cmdSocket;	//用于命令执行
};
