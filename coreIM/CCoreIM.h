#pragma once

#include <QObject>
#include <qurl.h>


class QThread;
class QTimer;
class CDistributedHashTable;
class CNetwork;


class CCoreIM : public QObject
{
	Q_OBJECT

public:
	
	~CCoreIM();
	static CCoreIM* getInstance();	
	void start();
private:
	CCoreIM(QObject* parent=nullptr);
	void onStarted();

private:
	static CCoreIM* m_pCoreIM;
	QThread* m_pThread;
	QTimer* m_pTimer;	
	CNetwork* m_pNet;
	CDistributedHashTable* m_pDht;
};
