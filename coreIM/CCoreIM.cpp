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
	m_pThread->start();
}

CCoreIM::CCoreIM(QObject* parent) : QObject(parent), m_pThread(new QThread), m_pTimer(new QTimer(this)),m_pDht(new CDistributedHashTable)
				,m_pNet(new CNetwork)
{
	connect(m_pDht, &CDistributedHashTable::sendDhtPacket, m_pNet, &CNetwork::sendPacket, Qt::DirectConnection);
	connect(m_pNet, &CNetwork::recieveDhtPacket, m_pDht, &CDistributedHashTable::onRecieveDhtPacket, Qt::DirectConnection);
	moveToThread(m_pThread);
	connect(m_pThread, &QThread::started, this, &CCoreIM::onStarted);
	
	
}

void CCoreIM::onStarted()
{
	//connect(m_pDht, &CDistributedHashTable::sendDhtPacket, m_pNet, &CNetwork::sendPacket);// , Qt::DirectConnection);
	m_pDht->addFriend(QByteArray::fromHex("F404ABAA1C99A9D37D61AB54898F56793E1DEF8BD46B1038B9D822E8460FAB67"));
	m_pNet->initNetwork(QHostAddress("127.0.0.1"));
	m_pDht->setClientID(QByteArray::fromHex("8E7D0B859922EF569298B4D261A8CCB5FEA14FB91ED412A7603A585A25698832"));
	IP_Port bootstrapIpport{QHostAddress("127.0.0.1"),33445 ,0};
	m_pDht->bootstrap(bootstrapIpport);
	m_pTimer->setInterval(30);
	connect(m_pTimer, &QTimer::timeout, m_pDht, &CDistributedHashTable::doDHT);
	m_pTimer->start();
}

CCoreIM::~CCoreIM()
{
}
