#include "HFtp.h"
#include "QNetworkAccessManager"
#include "QNetworkReply"
#include "QTimer"
#include "QTextCodec"
#include "QBuffer"
#include "QTextStream"

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
	m_socket->connectToHost(address, port);

	if (m_socket->waitForConnected() && m_socket->waitForReadyRead())
	{
		return readresp('2');
	}
	return false;
}

QList<QUrlInfo> HFtp::LIST(QString path)
{
	QBuffer buffer;

	auto count = FtpAccessRead("LIST -aL", transfermode::ascii, &buffer);

	return{};
}

QList<QUrlInfo> HFtp::NLST(QString path)
{
	QBuffer buffer;
	auto count = FtpAccessRead("LIST -aL", transfermode::ascii, &buffer);
	return{};
}

bool HFtp::login(QString username, QString password)
{
	if (!sendCMD(QString("USER %1").arg(username), '3'))
	{
		if (m_response.at(0) == '2') return 1;
		return 0;
	}
	return sendCMD(QString("PASS %1").arg(password),'2');
}

bool HFtp::reset(size_t offset)
{
	return sendCMD(QString("REST %1").arg(offset),'3');
}

bool HFtp::sendCMD(QString buffer, char expect)
{
	m_response = {};
	buffer += "\r\n\0";
	auto iw = m_socket->write(buffer.toUtf8());
	if (m_socket->waitForReadyRead())
	{
		return readresp(expect);
	}
	return false;
}

bool HFtp::readresp(char expect)
{
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

	return m_response[0] == expect;
}

QTcpSocket* HFtp::FtpOpenPasv(QString cmd, transfermode mode)
{
	if (!sendCMD("PASV", '2')) return 0;
	auto  lindex = m_response.indexOf('(');
	auto  rindex = m_response.indexOf(')', lindex + 1);
	if (lindex == -1 || rindex == -1)  return 0;
	auto l = QString::fromUtf8(m_response.mid(lindex, rindex)).split(',');

	//初始化端口
	QString ip = QString("%1.%2.%3.%4")
		.arg(l.value(0), l.value(1), l.value(2), l.value(3));

	short port = (l.value(4).toShort() << 8) | (l.value(5).toShort());


	//偏移
	if (!reset(m_offset)) return 0;

	if (!sendCMD(cmd, '3'))
		return 0;

	QScopedPointer<QTcpSocket> socket(new QTcpSocket(this));

	socket->connectToHost(QHostAddress(ip), port);

	if (!readresp('1'))
		return 0;

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

	if (!reset(m_offset)) return nullptr;

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
	if (!sendCMD(QString("TYPE %1").arg(mode), '2')) return 0;

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
	auto socket = FtpAccess(cmd, mode);
	if (socket == nullptr) 
		return 0;
	size_t sum = 0;
	while (socket->waitForReadyRead())
	{
		sum += iofile->write(socket->readAll());
	}
	return sum;
}

size_t HFtp::FtpAccessWrite(QString cmd, transfermode mode, QIODevice* iofile)
{
	if (iofile == nullptr)
		return 0;
	auto socket = FtpAccess(cmd, mode);
	if (socket == nullptr)
		return 0;
	size_t sum = 0;

	while (iofile->waitForReadyRead(30*1000))
	{
		sum += socket->write(iofile->read(10*1024*1024));
	}
	return sum;
}

QString HFtp::toString(const QByteArray& content)
{
	return m_code->toUnicode(content);
}

QByteArray HFtp::toByte(const QString& content)
{
	return m_code->fromUnicode(content);
}

