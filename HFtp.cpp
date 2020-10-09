#include "HFtp.h"
#include "QNetworkAccessManager"
#include "QNetworkReply"
#include "QTimer"
#include "QTextCodec"
#include "QBuffer"
#include "QTextStream"
#include <qdebug.h>
#include <iostream>

#ifdef _DEBUG
#include <QFile>
#endif // _DEBUG

HFtp::HFtp(QObject *parent)
	: QObject(parent)
{
	m_socket.reset(new QTcpSocket(this));
	m_code = QTextCodec::codecForName("utf-8");
}

HFtp::~HFtp()
{

}

bool HFtp::connectFTP(QString address, short port /*= 21*/)
{
	return connectFTP(QHostAddress(address), port);
}

bool HFtp::connectFTP(QHostAddress address, short port /*= 21*/)
{
	m_socket.reset(new QTcpSocket(this));

	m_socket->connectToHost(address, port);

	if (m_socket->waitForConnected())
	{
		if (readresp('2'))
		{
			return true;
		}
	}
	m_socket.reset(nullptr);

	return false;
}

QList<QUrlInfo> HFtp::list(QString path)
{
	QBuffer buffer;
	buffer.open(QBuffer::ReadWrite);
	QString cmd = "LIST";
	if (path.size())
		cmd += " " + path;
	auto count = FtpAccessRead(cmd, transfermode::ascii, &buffer);

	if (buffer.size()==0)
		return{};

	QList<QUrlInfo> ret;
	buffer.seek(0);

	while (!buffer.atEnd())
	{
		auto line = toString(buffer.readLine());
		if (line.isEmpty()) break;

		auto cols = line.splitRef(QRegExp(R"(\s+)"), QString::SkipEmptyParts);
		if (cols.size() < 9) continue;

		auto ioString = cols.at(0).toString().toLower();

		QUrlInfo info;
		info.isDir = ioString.at(0) == 'd';
		info.access = ioString.at(1) == 'r';

		//名字，第九列以后的全是
		info.name = line.mid(cols.at(8).position());

		//时间计算
		static QStringList monthMap = {
			"Jan", "Feb", "Mar", "Apr", "May", "Jun"
			, "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
		};

		//根据不同情况解析时间
#ifdef _DEBUG
		auto timeString = line.mid(cols.at(5).position(), cols.at(7).position() + cols.at(7).length() - cols.at(5).position());
#endif // _DEBUG
		int year = 0, month = 1, day = 1;

		QTime modifyTime;
		if (cols.at(7).indexOf(':') != -1)
		{
			modifyTime = QTime::fromString(cols.at(7).toString(), "HH:mm");
			year = QDate::currentDate().year();
		}
		else
		{
			year = cols.at(7).toInt();
		}

		month = monthMap.indexOf(cols.at(5).toString());
		day = cols.at(6).toInt();

		info.time = QDateTime(QDate(year,month,day), modifyTime);


		ret << info;
	}
			
	return std::move(ret);
}

QList<QUrlInfo> HFtp::nlst(QString path)
{
	QBuffer buffer;
	buffer.open(QIODevice::ReadWrite);
	QString cmd = "NLST";
	if (path.size())
		cmd += " " + path;
	auto count = FtpAccessRead(cmd, transfermode::ascii, &buffer);

	if (count == 0) return{};

#ifdef _DEBUG
	qDebug() << buffer.data();
#endif // _DEBUG
	buffer.seek(0);
	while (!buffer.atEnd())
	{
		auto content = buffer.readLine();
		if (content.isEmpty()) break;
		qInfo() << content;
		//TODO 分析列表
	}
	

	return{};
}

size_t HFtp::size(QString file)
{
	if (sendCMD("SIZE " + file, '2'))
	{
		return toString(m_response.mid(3)).toULongLong();
	}
	return 0;
}

size_t HFtp::get(QString remoteFile, QIODevice& outDevice)
{
	return FtpAccessRead("RETR " + remoteFile, transfermode::image, &outDevice)>0;
}

size_t HFtp::get(QString remoteFile, QString localFile)
{
	return get(remoteFile, QFile(localFile));
}

size_t HFtp::put(QIODevice& inDevice, QString remoteFile)
{
	return FtpAccessWrite("STOR " + remoteFile, transfermode::image, &inDevice);
}

size_t HFtp::put(QString localFile, QString remoteFile)
{
	return put(QFile(localFile), remoteFile);
}

bool HFtp::login(QString username, QString password)
{
	if (!sendCMD(QString("USER %1").arg(username), '3'))
	{
		if (m_response.at(0) == '2') return 1;
		return 0;
	}
	if(!sendCMD(QString("PASS %1").arg(password),'2'))
		return false;

	updateFtpCode();

	return true;
}

bool HFtp::reset(size_t offset)
{
	return sendCMD(QString("REST %1").arg(offset),'3');
}

bool HFtp::cwd(QString path)
{
	return sendCMD("CWD " + path,'2');
}

QString HFtp::pwd()
{
	if (!sendCMD("PWD", '2')) return{};

	auto indexl = m_response.indexOf('"');
	auto indexR = m_response.lastIndexOf('"', indexl);
	if (indexR <= indexl)
		return{};

	return toString(m_response.mid(indexl, indexR - indexl - 1));
}

bool HFtp::sendCMD(QString buffer, char expect)
{
	if (writeCMD(buffer))
	{
		return readresp(expect);
	}
	return false;
}

bool HFtp::writeCMD(QString buffer)
{
	m_response = {};
	buffer += "\r\n\0";
	auto iw = m_socket->write(toByte(buffer));
#ifdef _DEBUG
	qDebug() <<"<-- "<< buffer.trimmed();
#endif // _DEBUG
	return iw > 0;
}

bool HFtp::readresp(char expect)
{
	if (!m_socket->waitForReadyRead())
		return false;
	m_response = m_socket->readLine();

	if (m_response.size() < 3)
	{
		return false;
	}
	auto code = m_response.left(3);
	m_response[0] = code[0];
	m_response[1] = code[1];
	m_response[2] = code[2];

	if (m_response.size() >= 3 && m_response.at(3) == '-')
	{
		//过滤 200-之类的行
		code += ' ';

		QByteArray response;
		do
		{
			response = m_socket->readLine();
			if (response.isEmpty())
			{
				qInfo()<<("Control socket read failed");
				break;
			}
			m_response += response;
		} while (!response.startsWith(code));
	}
#ifdef _DEBUG
	qDebug() <<"--> "<< toString(m_response).trimmed();
#endif // _DEBUG
	return m_response[0] == expect;
}

QTcpSocket* HFtp::FtpOpenPasv(QString cmd, transfermode mode)
{
	if (!sendCMD("PASV", '2')) return 0;
	auto  lindex = m_response.indexOf('(');
	auto  rindex = m_response.indexOf(')', lindex + 1);
	if (lindex == -1 || rindex == -1)  return 0;
	auto l = QString::fromUtf8(m_response.mid(lindex + 1, rindex - lindex-1)).split(',');

	//初始化端口
	QString ip = QString("%1.%2.%3.%4")
		.arg(l.value(0), l.value(1), l.value(2), l.value(3));

	unsigned short port = (l.value(4).toShort() << 8) | (l.value(5).toShort());

	//偏移
	if (m_offset != 0 && !reset(m_offset)) return 0;

	if (!writeCMD(cmd))
		return nullptr;

	QScopedPointer<QTcpSocket> socket(new QTcpSocket(this));
	socket->connectToHost(QHostAddress(ip), port);
	if (!socket->waitForConnected())
		return nullptr;

	if (!readresp('1'))
		return nullptr;

	return socket.take();
}

QTcpSocket* HFtp::FtpOpenPort(QString cmd, transfermode mode)
{
	QScopedPointer<QTcpServer> tcpserver(new QTcpServer(this));
	if (!tcpserver->listen()) return nullptr;

	auto port = tcpserver->serverPort();
	auto ip = tcpserver->serverAddress().toString();

	QString portCmd = QString("PORT %1.%2")
		.arg(ip.replace('.', ','))
		.arg(port >> 8)
		.arg(port & 0xff);

	if (!sendCMD(portCmd, '2')) return nullptr;

	if (m_offset != 0 && !reset(m_offset)) return nullptr;

	if (!sendCMD(cmd, '1')) return nullptr;

	if (tcpserver->waitForNewConnection(-1))
	{
		auto tcpsocket = tcpserver->nextPendingConnection();
		tcpsocket->setParent(this);
		return tcpsocket;
	}
	return nullptr;
}

QTcpSocket* HFtp::FtpAccess(QString cmd, transfermode mode)
{
	if (!sendCMD(QString("TYPE %1").arg((QChar)mode), '2')) return 0;

	if (m_connmode == pasv)
	{
		//被动模式返回socket
		return FtpOpenPasv(cmd, mode);
	}

	if (m_connmode == port)
	{
		//被动模式返回socket
		return FtpOpenPort(cmd, mode);
	}

	return nullptr;
}

size_t HFtp::FtpAccessRead(QString cmd, transfermode mode, QIODevice* iofile)
{
	if (iofile == nullptr)
		return 0;

	if (!iofile->isOpen() && !iofile->open(QIODevice::WriteOnly))
		return 0;

	auto socket = FtpAccess(cmd, mode);
	if (socket == nullptr) 
		return 0;
	size_t sum = 0;
	while (socket->waitForReadyRead())
	{
		auto byte = socket->readAll();
		sum += iofile->write(byte);
	}

	//接收结束命令
	return readresp('2') ? sum : 0;
}

size_t HFtp::FtpAccessWrite(QString cmd, transfermode mode, QIODevice* iofile)
{
	if (iofile == nullptr)
		return 0;

	if (!iofile->isOpen() && !iofile->open(QIODevice::ReadOnly))
		return 0;

	const int readUnit = 16 * 1024;
	iofile->waitForReadyRead(30 * 1000);

	auto readbuffer = iofile->read(readUnit);
	if (readbuffer.isEmpty()) return 0;

	size_t sum = 0;
	{
		QScopedPointer<QTcpSocket> socket(FtpAccess(cmd, mode));
		if (socket == nullptr)
			return 0;

		do
		{
			//对齐读取
			auto writecount = socket->write(readbuffer);
			if (writecount <= 0) return sum;
			sum += writecount;
			iofile->waitForReadyRead(30 * 1000);
			readbuffer = iofile->read(readUnit);
		} while (readbuffer.size());
		socket->disconnected();
	}
	
	
	return readresp('2') ? sum : 0;
}

void HFtp::updateFtpCode()
{
	if (!sendCMD("FEAT", '2')) return;

	if (m_response.indexOf("UTF8") != -1)
	{
		if (sendCMD("OPTS UTF8 ON", '2'))
		{
			//设置比传输编码为utf8
			m_code = QTextCodec::codecForName("utf-8");
		}
	}

	
}

QString HFtp::toString(const QByteArray& content)const
{
	return m_code->toUnicode(content);
}

QByteArray HFtp::toByte(const QString& content)const
{
	return m_code->fromUnicode(content);
}

