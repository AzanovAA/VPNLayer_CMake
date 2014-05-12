//---------------------------------------------------------------------------
#pragma once
#include <QMutex>

#define SAFE_DELETE(x)  if (x) { delete x; x = NULL; }

class ALog
{
public:
	static void Out(QString str);
	static QMutex mutex_;
};
//---------------------------------------------------------------------------
