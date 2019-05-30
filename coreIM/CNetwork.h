#pragma once

#include <QObject>
#include <qhostaddress.h>
#include <qudpsocket.h>

const quint16 DEFAULT_PORT = 33445;

struct IP_Port
{
	QHostAddress ip;
	uint16_t port;
	uint16_t padding;
};

class CNetwork : public QObject
{
	Q_OBJECT

public:
	CNetwork(QObject *parent=nullptr);
	~CNetwork();



	//发送一个UDP包，返回发送数据的大小
	int sendPacket(const QByteArray& msg,const IP_Port& ipport);

	//接收一个UDP包，返回接收数据的大小
	//msg接收的数据，ipport接收到数据来源机器的IP和端口
	int recievePacket();

	//UDP绑定一个IP和端口
	bool initNetwork(QHostAddress ip, uint16_t port= DEFAULT_PORT);

	//关闭UDP socket
	void shutdownNetworking() { m_pUdpSocket->close(); }

signals:
	void recieveDhtPacket(QByteArray& msg, IP_Port& ipport);
private:
	QUdpSocket *m_pUdpSocket;
};
