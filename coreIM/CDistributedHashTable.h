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

	//通过过ID添加一个朋友，ID是一个32位的hash值
	int addfriend(QByteArray clientID);

	
	

	//从ipport机器的客户端引当前机器导客户端，发送一个 get nodes请求给位于ipport机器的客户端
	void bootstrap(IP_Port ipport);

signals:
	void pingRequest(QByteArray, IP_Port);
	void pingResponse(QByteArray, IP_Port);
	void getNode(QByteArray, IP_Port);
	void sendNode(QByteArray, IP_Port);

public slots:
	//收到一个DHT 包，触发这个函数
	bool  onRecieveDhtPacket(QByteArray packet, IP_Port source);

private:
	int getNodes(IP_Port ipport, QByteArray clientID);

	//看ipport在m_sendNodesList列表中吗
	bool isGettingNodes(IP_Port ipport, quint32 pingID);

	//把ipport添加到m_sendNodestList中，并创建一个随机pingID
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
