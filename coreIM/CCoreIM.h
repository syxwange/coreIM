#pragma once

#include <QObject>

class QThread;
class QTimer;
class CCoreIM : public QObject
{
	Q_OBJECT

public:
	
	~CCoreIM();
	static CCoreIM* getInstance();
private:
	CCoreIM(QObject* parent=nullptr);

	void onStarted();

private:
	static CCoreIM* m_pCoreIM;
	QThread* m_pThread;
	QTimer* m_pTimer;
};
