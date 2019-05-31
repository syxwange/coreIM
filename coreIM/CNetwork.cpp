#include "CNetwork.h"
#include <qnetworkdatagram.h>
#include <qdebug.h>

CNetwork::CNetwork(QObject *parent)	: QObject(parent),m_pUdpSocket(new QUdpSocket(this))
{
	qRegisterMetaType<IP_Port>("IP_Port");
}

CNetwork::~CNetwork()
{
	
}


int CNetwork::sendPacket(const QByteArray& msg,const  IP_Port& ipport)
{
	auto ret = m_pUdpSocket->isValid();
	if (!ret)
		qDebug() << "udp is busing";
	return m_pUdpSocket->writeDatagram(msg, ipport.ip,ipport.port);
	
}

int CNetwork::recievePacket()
{	
	IP_Port ipport{};
	auto data = m_pUdpSocket->receiveDatagram();
	//auto ret = m_pUdpSocket->readDatagram(msg.data(), msg.size(), &ipport.ip, &ipport.port);
	if (data.isNull())
		return -1;	
	ipport.ip = data.senderAddress();
	ipport.port = data.senderPort();
	auto msg = data.data();
	if (msg.isNull())
		return -1;
	emit recieveDhtPacket(msg, ipport);
	return msg.size();
}

bool CNetwork::initNetwork(QHostAddress ip, uint16_t port)
{	
	connect(m_pUdpSocket, &QUdpSocket::readyRead, this, &CNetwork::recievePacket,Qt::DirectConnection);
	//m_pUdpSocket->bind(ip, port);
	auto ret = m_pUdpSocket->bind(port, QAbstractSocket::ShareAddress);	
	return true;
}
