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
//The timeout after which a node is discarded completely.�ڵ㱻��ȫ�����ĳ�ʱ��
const int Kill_NODE_TIMEOUT = 300;
//ping interval in seconds for each node in our lists.�����б��е�ÿ���ڵ㣬ping���(����Ϊ��λ)��
const int PING_INTERVAL = 60;
//ping interval in seconds for each random sending of a get nodes request.ÿ���������sendNodesRequest�����ping���(����Ϊ��λ)��
const int  GET_NODE_INTERVAL = 10;
typedef struct
{
	char client_id[CLIENT_ID_SIZE];
	IP_Port ip_port;
	int64_t timestamp;
	int64_t last_pinged;
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
	int addFriend(QByteArray clientID);

	int delFriend(char* client_id);

	IP_Port getFriendIP(char* client_id);

	//ÿ����doDHT��������һ������������30�����ң�
	void doDHT();
	
	//ÿ60��Ping����ڵ��б��е�ÿ���ͻ�����ÿ��20�����б��е�����ýڵ㷢��һ��get�ڵ�����---doClose
	void doClosest();

	//ÿ60�뷢��һ��sendPingRequest�����m_friendLastGetnode������Ч��client_list��ʱ�䲻�Ǻܾ�ȷ
	//ÿ10�뷢��һ��sendNodesRequest�����m_friendLastGetnode�������client_list
	void doFriends();

	//��ipport�����Ŀͻ�������ǰ�������ͻ��ˣ�����һ�� get nodes�����λ��ipport�����Ŀͻ��ˣ������Լ�ID���͸�ipport�����Ŀͻ���
	void bootstrap(IP_Port ipport);

signals:
	//����һ��DHT��
	void sendDhtPacket(QByteArray, IP_Port);

public slots:
	//�յ�һ��DHT ���������������
	bool  onRecieveDhtPacket(QByteArray packet, IP_Port source);

	//�����յ�nodes����---handle_getnodes
	bool onNodesRequest(QByteArray packet, IP_Port source);

	//�����յ�nodes��ӦResponses    ---handle_sendnodes
	bool onNodesResponses(QByteArray packet, IP_Port source);

	//�����յ�ping ����   ---handle_pingreq
	bool onPingRequest(QByteArray packet, IP_Port source);

	//�����յ�ping ��ӦResponses   ---handle_pingres
	bool onPingResponses(QByteArray packet, IP_Port source);

private:
	//��ipport�Ļ���IDΪclientID�ͻ��˷���һ��Node����,�ɹ�����0�����ɹ�����-1 ---getnodes
	int sendNodesRequest(IP_Port ipport, QByteArray clientID);

	//Ϊ�յ���nodes������һ����Ӧ��---sendnodes
	int sendNodeResponses(IP_Port ipport, QByteArray clientID, quint32 pingID);

	//����һ��ping����---pingreq
	//[byte with value: 00 for request, 01 for response][random 4 byte (ping_id)][char array (client node_id), length=32 bytes]
	//ping_id = a random integer, the response must contain the exact same number as the requestping_id =һ�������������Ӧ���������������ȫ��ͬ������
	int sendPingRequest(IP_Port ipport);

	//����һ��ping ��Ӧ ---pingres
	int sendPingResoinses(IP_Port ip_port, quint32 ping_id);

	//��ipport��m_nodesRequestList�б�����--is_gettingnodes
	bool isSendNodesRequest(IP_Port ipport, quint32 pingID);

	//��һ���µ�Nodes�����ipport��ӵ�m_nodesRequestList�У�������һ�����pingID----add_gettingnodes
	//���⣺���������Ч��ʱ���m_sendNodesList��������û����ӣ���ô���Ŵ���
	int addNodesRequest(IP_Port ipport);	

	//����һ��pinged��m_pingsList��  ---add_pinging
	int addPinging(IP_Port ipport);

	//���Խ�����ip_port��client_id�Ŀͻ�����ӵ�m_friendsList�ͻ����б��m_closeClientlist�� ---addto_lists
	void addtoLists(IP_Port ip_port, char* client_id);

	//��m_closeClientlist��m_friendsList��client_list�в���MAX_SENT_NODES��8������ӽ�����NodesRequest����clientID�Ľڵ�: ---get_close_nodes
	//�����Ƿ���nodeList�У��������ҵ���������TODO:ʹ�����������Ч��
	int getCloseNodes(const QByteArray& clientID, NodeFormat* nodeList);

	//��鳤��Ϊlislength�б����Ƿ��Ѱ���clientID  ---client_in_nodelist
	bool clientInNodelist(const NodeFormat* list,const quint32 length,const QByteArray& clientID);

	//���client_id��ip_port�Ƿ���list��
	bool clientInList(ClientData* list, uint32_t length, char* client_id, IP_Port ip_port);

	// ---replace_bad
	bool replaceBadNode(ClientData* list, uint32_t length, char* client_id, IP_Port ip_port);//tested

	//---replace_good
	bool replaceGoodNode(ClientData* list, uint32_t length, char* client_id, IP_Port ip_port, char* comp_client_id);

	//�Ƚ�clientId��clientId1��clientId2  ----id_closest
	//����0����ͬ���ľ��룻����1��clientId1��������2��clientId2��
	//���⣺Ϊʲô����ô �жϣ�������
	int idClosest(const char* clientId,const char* clientId1,const char* clientId2);

	//ipport��pingID�Ƿ���m_pingsList�б��� ---is_pinging
	bool isPinging(IP_Port ipport, uint32_t pingID);
private:
	QByteArray m_selfClientID{32, 0x00};
	ClientData m_closeClientlist[LCLIENT_LIST]{};
	qint64 m_closeLastSendRequest = 0;

	Friend m_friendsList[MAX_FRIENDS]{};
	quint16 m_numFriends = 0;
	qint64 m_friendLastGetnode[MAX_FRIENDS]{};

	Pinged m_pingsList[LPING_ARRAY]{};	

	Pinged m_nodesRequestList[LSEND_NODES_ARRAY]{};
};
