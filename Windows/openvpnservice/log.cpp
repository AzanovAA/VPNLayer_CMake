//---------------------------------------------------------------------------
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include "log.h"

QMutex ALog::mutex_;
QString ALog::path_;

//---------------------------------------------------------------------------
void ALog::Out(QString str)
{
	QMutexLocker locker(&mutex_);

    QString fileName = path_ + "/log.txt";
	QFile file(fileName);
	if(file.open(QFile::Text | QFile::Append))
	{
		QDateTime dt = QDateTime::currentDateTime();
		file.seek(file.size());
		QTextStream out(&file);
		out << dt.toString("[hh:mm:ss] ") << str << "\n";
		file.close();
	}
}
