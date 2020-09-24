#include "stdafx.h"
#include "HFtp.h"
#include "QNetworkAccessManager"
#include "QNetworkReply"
#include "QTimer"

HFtp::HFtp(QObject *parent)
	: QTcpSocket(parent)
{
	m_networkManager.reset(new QNetworkAccessManager());
	m_networkManager->setNetworkAccessible(QNetworkAccessManager::Accessible);

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
	connectToHost(address, port);

	if (waitForConnected() && waitForReadyRead())
	{
		return readresp('2');
	}
	return false;
}

QList<QUrlInfo> HFtp::list(QString path)
{
	QUrl url;
	url.setScheme("ftp");
	url.setPort(m_port);
	url.setUserName(m_username);
	url.setPassword(m_password);
	url.setHost(m_address.toString());
	url.setPath(path);

	auto reply = m_networkManager->get(QNetworkRequest(url));

	QEventLoop lop;
	connect(reply, &QNetworkReply::finished, &lop, &QEventLoop::quit);
	QTimer::singleShot(30000, &lop, &QEventLoop::quit);
	lop.exec();

	auto temp = reply->readAll();
	qInfo() << temp;

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

bool HFtp::sendCMD(QString buffer, char expect)
{
	m_response = {};
	buffer += "\r\n";
	auto iw = write(buffer.toUtf8());
	if (waitForReadyRead())
	{
		return readresp(expect);
	}
	return false;
}

bool HFtp::readresp(char expect)
{
	auto response = readLine();

	if (response.size() < 3)
	{
		return false;
	}
	auto code = response.left(3);
	m_response[0] = code[0];
	m_response[1] = code[1];
	m_response[2] = code[2];

	if (response.size() >= 3 && response.at(3) == '-')
	{
		//过滤 200-之类的行
		code += ' ';

		do
		{
			response = readLine();
			if (response.isEmpty())
			{
				qInfo()<<("Control socket read failed");
			}
		} while (!response.startsWith(code));
	}

	return m_response[0] == expect;
}

void HFtp::FtpAccess(QIODevice* iofile, accesstype type, transfermode mode)
{
	
}

