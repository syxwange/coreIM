#include "CDistributedHashTable.h"

CDistributedHashTable::CDistributedHashTable(QObject *parent) : QObject(parent)
{
	m_network.initNetwork(QHostAddress(QHostAddress::LocalHost));	
}

CDistributedHashTable::~CDistributedHashTable()
{
}

int CDistributedHashTable::addfriend(QByteArray clientID)
{
	return 0;
}

void CDistributedHashTable::bootstrap(IP_Port ipport)
{
}

int CDistributedHashTable::getNodes(IP_Port ipport, QByteArray clientID)
{
	if (isGettingNodes(ipport, 0))
	{
		return -1;
	}
	int ping_id = addGettingNodes(ipport);

	if (ping_id == 0)
	{
		return -1;
	}
	QByteArray dataTemp(5 + CLIENT_ID_SIZE * 2, 0x00);	
	//getNode请求，格式data第一位是2
	dataTemp[0] = 2;
	memcpy(dataTemp.data()+1, &ping_id, sizeof(ping_id));
	memcpy(dataTemp.data() + 1 +sizeof(ping_id), m_selfClentID, CLIENT_ID_SIZE);
	memcpy(dataTemp.data() + 1 + sizeof(ping_id) + CLIENT_ID_SIZE, clientID, CLIENT_ID_SIZE);
	return  m_network.sendPacket(dataTemp,ipport);
}

bool CDistributedHashTable::isGettingNodes(IP_Port ipport, quint32 pingID)
{	
	auto tempTime = m_network.currentTime();
	int pinging = 0;
	for (auto& node : m_sendNodesList)
	{
		if (node.timestamp + PING_TIMEOUT*1000 > tempTime)
		{
			if (!ipport.ip.isNull() && node.ip_port.ip == ipport.ip && node.ip_port.port == ipport.port)
				pinging++;
			if (pingID != 0 && node.ping_id == pingID)
				pinging++;
			if (pinging == 0)
				return false;
		}
	}	
	return true;
}

int CDistributedHashTable::addGettingNodes(IP_Port ipport)
{
	auto tempTime = m_network.currentTime();
	auto pingID= m_network.randomInt();
	for (auto& node : m_sendNodesList)
	{
		if (tempTime-node.timestamp>PING_TIMEOUT*1000)
		{
			node.ip_port = ipport;
			node.ping_id = pingID;
			node.timestamp = tempTime;
			return pingID;
		}
	}
	return 0;
}

bool  CDistributedHashTable::onRecieveDhtPacket(QByteArray packet, IP_Port source)
{
	switch (packet[0])
	{
		case 0:
			emit pingRequest(packet, source);
		case 1:
			emit pingResponse(packet,  source);
		case 2:
			emit getNode(packet, source);
		case 3:
			emit sendNode(packet, source);
		default:
			return false;
	}
	return true;
}
