#pragma once

#include <QObject>
#include <qurl.h>
#include "CNetwork.h"

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

	//找到服务节点，（登录或启动）IM
	void bootstrap(IP_Port ipport);
	
	void addFriend(QByteArray clientID);
	void initNetwork(QHostAddress IP, quint16 port=33445);
	void setClientID(QByteArray clientID);

private:
	CCoreIM(QObject* parent=nullptr);
	void onStarted();	

private:
	static CCoreIM* m_pCoreIM;
	QThread* m_pThread;
	QTimer* m_pTimer;	
	CNetwork* m_pNet{nullptr};
	CDistributedHashTable* m_pDht{nullptr};
};
