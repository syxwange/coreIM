#include "CCoreIM.h"
#include <qthread.h>
#include <qtimer.h>
#include "CDistributedHashTable.h"
#include "CNetwork.h"
#include <qdebug.h>
////F404ABAA1C99A9D37D61AB54898F56793E1DEF8BD46B1038B9D822E8460FAB67
////8E7D0B859922EF569298B4D261A8CCB5FEA14FB91ED412A7603A585A25698832
CCoreIM* CCoreIM::m_pCoreIM{ nullptr };





CCoreIM* CCoreIM::getInstance()
{
	if (!m_pCoreIM)
	{
		m_pCoreIM = new CCoreIM;
	}
	return m_pCoreIM;
}

void CCoreIM::start()
{
	moveToThread(m_pThread);
	m_pThread->start();
}

void  CCoreIM::bootstrap(IP_Port ipport)
{
	m_pDht->bootstrap(ipport);
}

void CCoreIM::addFriend(QByteArray clientID)
{
	/*m_pNet->initNetwork(QHostAddress::LocalHost);
	IP_Port ipport{ QHostAddress("127.0.0.1"),33445,0 };
	m_pDht->bootstrap(ipport);*/
	m_pDht->addFriend(clientID);
}

void CCoreIM::initNetwork(QHostAddress IP,quint16 port)
{
	m_pNet->initNetwork(IP, port);
}

void CCoreIM::setClientID(QByteArray clientID)
{
	m_pDht->setClientID(clientID);
}

CCoreIM::CCoreIM(QObject* parent) : QObject(parent), m_pThread(new QThread), m_pTimer(new QTimer(this)),m_pDht(new CDistributedHashTable(this))
			,m_pNet(new CNetwork(this))
{	
	m_pThread->setObjectName("coreIM");
	connect(m_pThread, &QThread::started, this, &CCoreIM::onStarted);	
	connect(m_pDht, &CDistributedHashTable::sendDhtPacket, m_pNet, &CNetwork::sendPacket);
	connect(m_pNet, &CNetwork::recieveDhtPacket, m_pDht, &CDistributedHashTable::onRecieveDhtPacket);
	
}

void CCoreIM::onStarted()
{	
	//m_pNet->initNetwork(QHostAddress::LocalHost);
	
	m_pTimer->setInterval(30);
	connect(m_pTimer, &QTimer::timeout, m_pDht, &CDistributedHashTable::doDHT);
	m_pTimer->start();
}

CCoreIM::~CCoreIM()
{
	m_pThread->quit();
	m_pThread->wait();
	m_pThread->deleteLater();
}
