#include "showlog.h"
#include <QFile>

ShowLog::ShowLog(QWidget *parent)
	: QDialog(parent, Qt::WindowTitleHint)
{
	ui.setupUi(this);

	QString pathToLog = QApplication::applicationDirPath() + "/log.txt";
	QFile file(pathToLog);

	if (file.open(QIODevice::ReadOnly))
	{
		QByteArray arr = file.readAll();
		ui.textBrowser->setPlainText(arr);
		file.close();
	}
}

ShowLog::~ShowLog()
{

}
