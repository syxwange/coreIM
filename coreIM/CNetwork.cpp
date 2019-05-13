#include "CNetwork.h"
#include <qdatetime.h>
#include <random>
#include <qnetworkdatagram.h>

CNetwork::CNetwork(QObject *parent)	: QObject(parent),m_pUdpSocket(new QUdpSocket(this))
{
}

CNetwork::~CNetwork()
{
	
}

int64_t CNetwork::currentTime()
{
	return QDateTime::currentDateTime().toMSecsSinceEpoch();	
}


int CNetwork::randomInt()
{	
	std::default_random_engine randEngine((std::random_device())());
	std::uniform_int_distribution<int> distribution;
	return distribution(randEngine);	
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
