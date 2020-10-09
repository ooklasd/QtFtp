#include <QtCore/QCoreApplication>
#include "../HFtp.h"
#include <iostream>
#include <afxcom_.h>
#include <QTemporaryFile>
#include <QFileInfo>
#include "QBuffer"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	HFtp ftp;
	if (!ftp.connectFTP("192.168.0.108", 21))
		return 0;

	if (!ftp.login("guosm", ""))
		return 0;

	if (!ftp.cwd()) return 0;

	//auto files1 = ftp.NLST("/");
	auto files2 = ftp.list("/");

	ASSERT(19701636 == ftp.size("./citra-setup-windows.exe"));
	ASSERT(36939016 == ftp.size("/Visual Assist X 10.9 builds 2333/VA_X_Setup2333_0.exe"));
	ASSERT(999624 == ftp.size(QString::fromLocal8Bit("/War3 ±ù·âÍõ×ù_1@580699.exe")));

	QTemporaryFile file;
	file.open();
	ASSERT(ftp.get("citra-setup-windows.exe", file));
	ASSERT(file.size() == 19701636);
	
	file.seek(0);
	ftp.mkd("/temp/temp2");
	ASSERT(ftp.put(file, "/temp/ftptemp.txt") == 19701636);

	return 0;
}
