#pragma once

#include <QThread>

class DownloadFile : public QThread
{
	Q_OBJECT

public:
    DownloadFile(QString url, QObject *parent = NULL);

	// call only after thread finished signal
    QString data() { return data_; }
	bool success() { return success_; }

protected:
	virtual void run();
private:
	bool	success_;
    QString data_;
    QString url_;
};

