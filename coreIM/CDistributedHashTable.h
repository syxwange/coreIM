#pragma once

#include <QObject>
#include "CNetwork.h"

const int  CLIENT_ID_SIZE = 32;
const int MAX_FRIEND_CLIENTS = 8;
const int MAX_FRIENDS = 256;
const int LPING_ARRAY = 128;
const int LSEND_NODES_ARRAY = LPING_ARRAY / 2;
const int  LCLIENT_LENGTH = 128;
const int LCLIENT_NODES = 8;
const int LCLIENT_LIST = 32;// LCLIENT_LENGTH* LCLIENT_NODES;
const int PING_TIMEOUT = 5;


typedef struct
{
	char client_id[CLIENT_ID_SIZE];
	IP_Port ip_port;
	uint32_t timestamp;
	uint32_t last_pinged;
}ClientData;

typedef struct
{
	char client_id[CLIENT_ID_SIZE];
	ClientData client_list[MAX_FRIEND_CLIENTS];
}Friend;

typedef struct
{
	char client_id[CLIENT_ID_SIZE];
	IP_Port ip_port;
}NodeFormat;

typedef struct
{
	IP_Port ip_port;
	uint32_t ping_id;
	qint64 timestamp;
}Pinged;


class CDistributedHashTable : public QObject
{
	Q_OBJECT

public:
	CDistributedHashTable(QObject *parent=nullptr);
	~CDistributedHashTable();

	//ͨ����ID���һ�����ѣ�ID��һ��32λ��hashֵ
	int addfriend(QByteArray clientID);

	
	

	//��ipport�����Ŀͻ�������ǰ�������ͻ��ˣ�����һ�� get nodes�����λ��ipport�����Ŀͻ���
	void bootstrap(IP_Port ipport);

signals:
	void pingRequest(QByteArray, IP_Port);
	void pingResponse(QByteArray, IP_Port);
	void getNode(QByteArray, IP_Port);
	void sendNode(QByteArray, IP_Port);

public slots:
	//�յ�һ��DHT ���������������
	bool  onRecieveDhtPacket(QByteArray packet, IP_Port source);

private:
	int getNodes(IP_Port ipport, QByteArray clientID);

	//��ipport��m_sendNodesList�б�����
	bool isGettingNodes(IP_Port ipport, quint32 pingID);

	//��ipport��ӵ�m_sendNodestList�У�������һ�����pingID
	int addGettingNodes(IP_Port ipport);

private:
	QByteArray m_selfClentID{32, 0x00};
	ClientData closeClientlist[LCLIENT_LIST];

	Friend m_friendsList[MAX_FRIENDS]{};
	quint16 m_numFriends = 0;

	Pinged m_pingsList[LPING_ARRAY]{};
	quint16 m_numPings = 0;

	Pinged m_sendNodesList[LSEND_NODES_ARRAY];

	CNetwork m_network{};
};
