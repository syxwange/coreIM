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
const int MAX_SENT_NODES = 8;
const int BAD_NODE_TIMEOUT = 130;

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

	//每秒中doDHT函数运行一定次数（估计30次左右）
	void doDHT();
	
	//每60秒Ping最近节点列表中的每个客户机。每隔20秒向列表中的随机好节点发送一个get节点请求。
	void doClosest();

	void doFriends();

	//从ipport机器的客户端引当前机器导客户端，发送一个 get nodes请求给位于ipport机器的客户端，并把自己ID发送给ipport机器的客户端
	void bootstrap(IP_Port ipport);

signals:
	void pingRequest(QByteArray, IP_Port);
	void pingResponse(QByteArray, IP_Port);
	void nodesRequest(QByteArray, IP_Port);
	void nodesResponses(QByteArray, IP_Port);
	//发送一个DHT包
	void sendDhtPacket(QByteArray, IP_Port);

public slots:
	//收到一个DHT 包，触发这个函数
	bool  onRecieveDhtPacket(QByteArray packet, IP_Port source);

	//处理收到nodes请求---handle_getnodes
	bool onNodesRequest(QByteArray packet, IP_Port source);

private:
	//向ipport的机器ID为clientID客户端发送一个Node请求,成功返回0，不成功返回-1 ---getnodes
	int sendNodesRequest(IP_Port ipport, QByteArray clientID);

	//看ipport在m_nodesRequestList列表中吗--is_gettingnodes
	bool isSendNodesRequest(IP_Port ipport, quint32 pingID);

	//把一个新的Nodes请求的ipport添加到m_nodesRequestList中，并创建一个随机pingID----add_gettingnodes
	//问题：如果都是有效的时间戳m_sendNodesList是满的则没有添加，怎么最优处理？
	int addNodesRequest(IP_Port ipport);

	//为收到的nodes请求发送一个响应。---sendnodes
	int sendNodeResponses(IP_Port ipport,QByteArray clientID,quint32 pingID);

	//发送一个ping请求。---pingreq
	int sendPingRequest(IP_Port ipport);

	//在m_closeClientlist和m_friendsList的client_list中查找MAX_SENT_NODES（8）个最接近发送NodesRequest请求clientID的节点: ---get_close_nodes
	//将它们放入nodeList中，并返回找到的数量。TODO:使这个函数更有效。
	int getCloseNodes(const QByteArray& clientID, NodeFormat* nodeList);

	//检查长度为lislength列表中是否已包含clientID  ---client_in_nodelist
	bool clientInNodelist(const NodeFormat* list,const quint32 length,const QByteArray& clientID);

	//比较clientId，clientId1，clientId2  ----id_closest
	//返回0都是同样的距离；返回1则clientId1近；返回2则clientId2近
	//问题：为什么是这么 判断？？？？
	int idClosest(const char* clientId,const char* clientId1,const char* clientId2);

	//ipport和pingID是否在m_pingsList列表中
	bool isPinging(IP_Port ipport, uint32_t pingID);
private:
	QByteArray m_selfClientID{32, 0x00};
	ClientData m_closeClientlist[LCLIENT_LIST];

	Friend m_friendsList[MAX_FRIENDS]{};
	quint16 m_numFriends = 0;

	Pinged m_pingsList[LPING_ARRAY]{};	

	Pinged m_nodesRequestList[LSEND_NODES_ARRAY];

	CNetwork m_network{};
};
