#include "CDistributedHashTable.h"
#include "CUtil.h"

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

 
{
	doClosest();
	doFriends();
}

void CDistributedHashTable::doClosest()
{
	uint32_t i;
	auto tempTime = CUtil::currentTime();
	int numNodes = 0;	

	for (auto& node : m_closeClientlist)
	{
		if (tempTime - node.timestamp < Kill_NODE_TIMEOUT*1000)//if node is not dead.
		{
			if (tempTime - node.last_pinged >= PING_INTERVAL*1000)
			{
				sendPingRequest(node.ip_port);
				node.last_pinged = tempTime;
			}
			if (tempTime - node.timestamp < BAD_NODE_TIMEOUT * 1000)
				numNodes++;
		}
	}

	if (m_closeLastSendRequest + GET_NODE_INTERVAL*1000 <= tempTime&& numNodes != 0)
	{
		auto n = CUtil::randomInt(numNodes);
		sendNodesRequest(m_closeClientlist[n].ip_port,QByteArray::fromRawData(m_closeClientlist[n].client_id,32) );
		m_closeLastSendRequest = tempTime;
	}
}

void CDistributedHashTable::doFriends()
{	
	auto tempTime = CUtil::currentTime();	
	uint32_t randNode=0;	
	for (int i = 0; i < m_numFriends; i++)
	{
		uint32_t numNodes = 0;
		for (auto& node : m_friendsList[i].client_list)
		{
			//node的timestamp没有超过杀死节点的时间
			if (tempTime - node.timestamp < Kill_NODE_TIMEOUT * 1000)
			{
				//node节点的last_pinged超过 ping间隔的时间，每60秒发送一个sendPingRequest请求。
				if (tempTime - node.last_pinged >= PING_INTERVAL * 1000)
				{
					sendPingRequest(node.ip_port);
					node.last_pinged = tempTime;
				}
			}
			if (tempTime - node.timestamp < BAD_NODE_TIMEOUT * 1000)
			{				
				numNodes++;
			}
		}
		//todo :可以合并到m_friendsList结构中。每10秒发送一个sendNodesRequest请求给m_friendLastGetnode中随机的client_list
		if (m_friendLastGetnode[i] + GET_NODE_INTERVAL*1000 <= tempTime && numNodes != 0)
		{			
			randNode =CUtil::randomInt(numNodes);		
			sendNodesRequest(m_friendsList[i].client_list[randNode].ip_port, QByteArray::fromRawData(m_friendsList[i].client_list[randNode].client_id,32));
			m_friendLastGetnode[i] = tempTime;
		}
	}
}

void CDistributedHashTable::bootstrap(IP_Port ipport)
{
	sendNodesRequest(ipport, m_selfClientID);
}

bool CDistributedHashTable::onNodesRequest(QByteArray packet, IP_Port source)
{
	if(packet.size()!= (5 + CLIENT_ID_SIZE * 2))
		return false;
	quint32 clientID=0;
	memcpy(&clientID, packet.data() + 1, sizeof(clientID));
	if (isSendNodesRequest({}, clientID))
		return false;
	sendNodeResponses(source, packet.replace(5, CLIENT_ID_SIZE), clientID);

	sendPingRequest(source);
	return true;
}

int CDistributedHashTable::sendNodesRequest(IP_Port ipport, QByteArray clientID)
{
	if (isSendNodesRequest(ipport, 0))
		return -1;

	int ping_id = addNodesRequest(ipport);

	if (ping_id == 0)
		return -1;

	QByteArray dataTemp(5 + CLIENT_ID_SIZE * 2, 0x00);	
	//NodesRequest请求，格式是 [byte with value: 02][random 4 byte (ping_id)][char array (自己的ID), length=32 bytes]
	//[char array: 目标机客户端ID(node_id of which we want the ip), length=32 bytes]
	dataTemp[0] = 2;
	memcpy(dataTemp.data()+1, &ping_id, sizeof(ping_id));
	memcpy(dataTemp.data() + 1 +sizeof(ping_id), m_selfClientID, CLIENT_ID_SIZE);
	memcpy(dataTemp.data() + 1 + sizeof(ping_id) + CLIENT_ID_SIZE, clientID, CLIENT_ID_SIZE);
	emit sendDhtPacket(dataTemp,ipport);
	return 0;
}

bool CDistributedHashTable::isSendNodesRequest(IP_Port ipport, quint32 pingID)
{	
	auto tempTime = CUtil::currentTime();
	int pinging = 0;
	for (auto& node : m_nodesRequestList)
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

int CDistributedHashTable::addNodesRequest(IP_Port ipport)
{
	auto tempTime = CUtil::currentTime();
	auto pingID= CUtil::randomInt();
	for (auto& node : m_nodesRequestList)
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

int CDistributedHashTable::sendNodeResponses(IP_Port ipport, QByteArray clientID, quint32 pingID)
{
	QByteArray dataTemp(5 + CLIENT_ID_SIZE +  sizeof(NodeFormat) * MAX_SENT_NODES, 0x00);
	
	NodeFormat nodesList[MAX_SENT_NODES];
	int numNodes = getCloseNodes(clientID, nodesList);
	if (numNodes == 0)
		return 0;

	//Send_nodes (response):发送nodesResponse包格式是 [byte with value: 03][random 4 byte (ping_id)][char array  (client node_id), length=32 bytes]
	//[Nodes in node format, length=40 * (number of nodes (maximum of 8 nodes)) bytes]
	dataTemp[0] = 3;
	memcpy(dataTemp.data() + 1, &pingID, 4);
	memcpy(dataTemp.data() + 5, m_selfClientID, CLIENT_ID_SIZE);
	memcpy(dataTemp.data() + 5 + CLIENT_ID_SIZE, nodesList, numNodes * (CLIENT_ID_SIZE + sizeof(IP_Port)));

	emit sendDhtPacket(dataTemp, ipport);
	return 0;
}

int CDistributedHashTable::sendPingRequest(IP_Port ipport)
{
	if (isPinging(ipport, 0))
		return 1;

	int pingID = addPinging(ipport);
	if (pingID == 0)
		return 1;
	//[byte with value: 00 for request, 01 for response][random 4 byte (ping_id)][char array (client node_id), length=32 bytes]
	//ping_id = a random integer, the response must contain the exact same number as the request
	QByteArray data(5 + CLIENT_ID_SIZE, 0x00);
	data[0] = 0;
	memcpy(data.data() + 1, &pingID, 4);
	memcpy(data.data() + 5, m_selfClientID, CLIENT_ID_SIZE);
	emit sendDhtPacket(data, ipport);
	return 0;
}

///当都是有效时间会不会没有添加成功。
int CDistributedHashTable::addPinging(IP_Port ipport)
{
	auto tempTime = CUtil::currentTime();
	for (int i = 0; i < PING_TIMEOUT; i++)
	{
		for (auto& ping : m_pingsList)
		{
			if (tempTime - ping.timestamp > (PING_TIMEOUT - i) * 1000)
			{
				ping.timestamp = tempTime;
				ping.ip_port = ipport;
				ping.ping_id = CUtil::randomInt();
				return ping.ping_id;
			}
		}		
	}
	return 0;
}

int CDistributedHashTable::getCloseNodes(const QByteArray& clientID, NodeFormat* nodeList)
{
	int numNodes = 0;
	auto timeTemp = CUtil::currentTime();
	for (auto& node : m_closeClientlist)
	{
		//如果node是有效的，且没有在nodelist中
		if (timeTemp - node.timestamp < BAD_NODE_TIMEOUT * 1000 && !clientInNodelist(nodeList, MAX_SENT_NODES, node.client_id))
		{
			if (numNodes < MAX_SENT_NODES)
			{				
				memcpy( nodeList[numNodes].client_id ,node.client_id, CLIENT_ID_SIZE);
				nodeList[numNodes].ip_port = node.ip_port;				
				numNodes++;
			}
			else for (int j = 0; j < MAX_SENT_NODES; j++)
			{
				if (idClosest(clientID.data(), nodeList[j].client_id, node.client_id) == 2)
				{
					memcpy(nodeList[j].client_id, node.client_id, CLIENT_ID_SIZE);
					nodeList[j].ip_port = node.ip_port;
					break;
				}
			}
		}
	}

	for (int i = 0; i < m_numFriends; i++)
	{
		for (int j = 0; j < MAX_FRIEND_CLIENTS; j++)
		{
			if (timeTemp-m_friendsList[i].client_list[j].timestamp <BAD_NODE_TIMEOUT *1000 &&
				!clientInNodelist(nodeList, MAX_SENT_NODES, m_friendsList[i].client_list[j].client_id))
				//if node is good and not already in list.
			{
				if (numNodes < MAX_SENT_NODES)
				{
					memcpy(nodeList[numNodes].client_id, m_friendsList[i].client_list[j].client_id, CLIENT_ID_SIZE);
					nodeList[numNodes].ip_port = m_friendsList[i].client_list[j].ip_port;
					numNodes++;
				}
				else for (int k = 0; k < MAX_SENT_NODES; k++)
				{
					if (idClosest(clientID, nodeList[k].client_id, m_friendsList[i].client_list[j].client_id) == 2)
					{
						memcpy(nodeList[k].client_id, m_friendsList[i].client_list[j].client_id, CLIENT_ID_SIZE);
						nodeList[k].ip_port = m_friendsList[i].client_list[j].ip_port;
						break;
					}
				}
			}
		}
	}
	return numNodes;
}

bool CDistributedHashTable::clientInNodelist(const NodeFormat* list, const quint32 length, const QByteArray& clientID)
{
	for (int i = 0; i < length; i++)
	{
		if (list[i].client_id == clientID)
			return true;	
	}	
	return false;
}

int CDistributedHashTable::idClosest(const char* clientId,const char* clientId1,const char* clientId2)
{	
	for (int i = 0; i < CLIENT_ID_SIZE; i++)
	{
		if (abs(clientId[i] ^ clientId1[i]) < abs(clientId[i] ^ clientId2[i]))
		{
			return 1;
		}
		else if (abs(clientId[i] ^ clientId1[i]) > abs(clientId[i] ^ clientId2[i]))
		{
			return 2;
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
			emit nodesRequest(packet, source);
		case 3:
			emit nodesResponses(packet, source);
		default:
			return false;
	}
	return true;
}

bool CDistributedHashTable::isPinging(IP_Port ipport, uint32_t pingID)
{
	uint8_t pinging=0;
	auto tempTime = CUtil::currentTime();

	for (auto& pings : m_pingsList)
	{
		if (tempTime - pings.timestamp > PING_TIMEOUT * 1000)
		{
			if (!ipport.ip.isNull()&&pings.ip_port.ip==ipport.ip&&pings.ip_port.port==ipport.port)			
				pinging++;

			if (pingID != 0&&pings.ping_id==pingID)
				pinging++;

			if (pinging == (pingID != 0) + !ipport.ip.isNull())
				return false;
		}
	}
	return true;
}


