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

	//���غ��뼶ʱ���
	int64_t  currentTime();

	//�����������
	int randomInt();

	//����һ��UDP�������ط������ݵĴ�С
	int sendPacket(const QByteArray& msg,const IP_Port& ipport);

	//����һ��UDP�������ؽ������ݵĴ�С
	//msg���յ����ݣ�ipport���յ�������Դ������IP�Ͷ˿�
	int recievePacket( QByteArray& msg,  IP_Port& ipport);

	//UDP��һ��IP�Ͷ˿�
	bool initNetwork(QHostAddress ip, uint16_t port= DEFAULT_PORT);

	//�ر�UDP socket
	void shutdownNetworking() { m_pUdpSocket->close(); }
private:
	QUdpSocket *m_pUdpSocket;
};