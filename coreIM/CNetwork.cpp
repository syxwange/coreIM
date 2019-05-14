#include "CNetwork.h"
#include <qnetworkdatagram.h>

CNetwork::CNetwork(QObject *parent)	: QObject(parent),m_pUdpSocket(new QUdpSocket(this))
{
}

CNetwork::~CNetwork()
{
	
}


int CNetwork::sendPacket(const QByteArray& msg,const  IP_Port& ipport)
{
	return m_pUdpSocket->writeDatagram(msg, ipport.ip,ipport.port);
}

int CNetwork::recievePacket(QByteArray& msg, IP_Port& ipport)
{
	auto ret = m_pUdpSocket->readDatagram(msg.data(), msg.size(), &ipport.ip, &ipport.port);
	if (-1 == ret)
		return -1;
	msg.resize(ret);
	return ret;
}

bool CNetwork::initNetwork(QHostAddress ip, uint16_t port)
{
	if(!m_pUdpSocket->isValid())
		return false;
	return m_pUdpSocket->bind(ip, port);
}
