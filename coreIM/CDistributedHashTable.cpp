#include "CDistributedHashTable.h"
#include "CUtil.h"

CDistributedHashTable::CDistributedHashTable(QObject *parent) : QObject(parent)
{
	
}

CDistributedHashTable::~CDistributedHashTable()
{
}

int CDistributedHashTable::addFriend(QByteArray clientID)
{
	if (MAX_FRIENDS > m_numFriends)
	{
		memcpy(m_friendsList[m_numFriends].client_id, clientID, CLIENT_ID_SIZE);
		m_numFriends++;
		return 0;
	}
	return 1;
}

int CDistributedHashTable::delFriend(char* client_id)
{
	uint32_t i;
	for (i = 0; i < m_numFriends; i++)
	{
		if (memcmp(m_friendsList[i].client_id, client_id, CLIENT_ID_SIZE) == 0)//Equal
		{
			m_numFriends--;
			m_friendsList[i] = m_friendsList[m_numFriends];			
			m_friendsList[m_numFriends] = {};
			return 0;
		}
	}
	return 1;
}

IP_Port CDistributedHashTable::getFriendIP(char* client_id)
{	
	IP_Port empty = { };

	for (int i = 0; i < m_numFriends; i++)
	{
		if (memcmp(m_friendsList[i].client_id, client_id, CLIENT_ID_SIZE) == 0)//Equal
		{
			for (auto& node : m_friendsList[i].client_list)
			{
				if (memcmp(node.client_id, client_id, CLIENT_ID_SIZE) == 0)
					return node.ip_port;
			}			
			return empty;
		}
	}	
	return empty;	
}

void CDistributedHashTable::doDHT()
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
	quint32 pingID=0;
	memcpy(&pingID, packet.data() + 1, sizeof(pingID));
	if (isSendedNodesRequest({}, pingID))
		return false;
	sendNodeResponses(source, packet.replace(5, CLIENT_ID_SIZE), pingID);

	sendPingRequest(source);
	return true;
}

bool CDistributedHashTable::onNodesResponses(QByteArray packet, IP_Port source)
{
	if (packet.size() > (5 + CLIENT_ID_SIZE + MAX_SENT_NODES * (CLIENT_ID_SIZE + sizeof(IP_Port))) ||
		(packet.size() - 5 - CLIENT_ID_SIZE) % (CLIENT_ID_SIZE + sizeof(IP_Port)) != 0)
	{
		return false;
	}
	uint32_t num_nodes = (packet.size() - 5 - CLIENT_ID_SIZE) / (CLIENT_ID_SIZE + sizeof(IP_Port));
	uint32_t ping_id;

	memcpy(&ping_id, packet.data() + 1, 4);
	if (!isSendedNodesRequest(source, ping_id))
		return false;

	NodeFormat nodes_list[MAX_SENT_NODES]{};
	memcpy(nodes_list, packet.data() + 5 + CLIENT_ID_SIZE, num_nodes * (CLIENT_ID_SIZE + sizeof(IP_Port)));

	for (int i = 0; i < num_nodes; i++)
	{
		sendPingRequest(nodes_list[i].ip_port);
	}
	addtoLists(source, packet.data() + 5);
	return true;
}

////有问题吗？
bool CDistributedHashTable::onPingRequest(QByteArray packet, IP_Port source)
{
	if (packet.size() != 5 + CLIENT_ID_SIZE)
	{
		return false;
	}
	int ping_id;
	memcpy(&ping_id, packet.data() + 1, 4);
	IP_Port bad_ip = {};

	if (isPinging(bad_ip, ping_id))//check if packet is from ourself.
	{
		return false;
	}
	sendPingResoinses(source, ping_id);
	sendPingRequest(source);
	return true;
}

bool CDistributedHashTable::onPingResponses(QByteArray packet, IP_Port source)
{
	if (packet.size() != (5 + CLIENT_ID_SIZE))
	{
		return false;
	}
	uint32_t ping_id;

	memcpy(&ping_id, packet.data() + 1, 4);
	if (isPinging(source, ping_id))
	{
		addtoLists(source, packet.data() + 5);
		return true;
	}
	return false;
}

int CDistributedHashTable::sendNodesRequest(IP_Port ipport, QByteArray clientID)
{
	if (isSendedNodesRequest(ipport, 0))
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

bool CDistributedHashTable::isSendedNodesRequest(IP_Port ipport, quint32 pingID)
{	
	auto tempTime = CUtil::currentTime();	
	for (auto& node : m_nodesRequestList)
	{
		if (node.timestamp + PING_TIMEOUT*1000 > tempTime)
		{
			if (!ipport.ip.isNull() && node.ip_port.ip == ipport.ip && node.ip_port.port == ipport.port)
				return true;
			if (pingID != 0 && node.ping_id == pingID)
				return true;
		}
	}	
	return false;
}

int CDistributedHashTable::addNodesRequest(IP_Port ipport)
{
	auto tempTime = CUtil::currentTime();	
	for (int i = 0; i < PING_TIMEOUT; i++)
	{
		for (auto& node : m_nodesRequestList)
		{
			if (tempTime - node.timestamp > (PING_TIMEOUT-i)* 1000)
			{
				node.ip_port = ipport;
				node.ping_id = CUtil::randomInt();
				node.timestamp = tempTime;
				return node.ping_id;
			}
		}
	}	
	return 0;
}

int CDistributedHashTable::sendNodeResponses(IP_Port ipport, QByteArray clientID, quint32 pingID)
{
	QByteArray dataTemp(5 + CLIENT_ID_SIZE +  sizeof(NodeFormat) * MAX_SENT_NODES, 0x00);
	
	NodeFormat nodesList[MAX_SENT_NODES]{};
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
	memcpy(data.data() + 5, m_selfClientID.data(), CLIENT_ID_SIZE);
	emit sendDhtPacket(data, ipport);
	return 0;
}

int CDistributedHashTable::sendPingResoinses(IP_Port ip_port, quint32 ping_id)
{
	//[byte with value: 00 for request, 01 for response][random 4 byte (ping_id)][char array (client node_id), length=32 bytes]
	//ping_id = a random integer, the response must contain the exact same number as the request
	QByteArray data(5 + CLIENT_ID_SIZE,0x00);
	data[0] = 1;
	memcpy(data.data() + 1, &ping_id, 4);
	memcpy(data.data() + 5, m_selfClientID.data(), CLIENT_ID_SIZE);
	emit sendDhtPacket(data, ip_port);
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

void CDistributedHashTable::addtoLists(IP_Port ip_port, char* client_id)
{
	//NOTE: current behaviour if there are two clients with the same id is to only keep one (the first one)
	if (!clientInList(m_closeClientlist, LCLIENT_LIST, client_id, ip_port))
	{

		if (replaceBadNode(m_closeClientlist, LCLIENT_LIST, client_id, ip_port))
		{
			//if we can't replace bad nodes we try replacing good ones
			replaceGoodNode(m_closeClientlist, LCLIENT_LIST, client_id, ip_port, m_selfClientID.data());
		}

	}
	for (int i = 0; i < m_numFriends; i++)
	{
		if (!clientInList(m_friendsList[i].client_list, MAX_FRIEND_CLIENTS, client_id, ip_port))
		{

			if (replaceBadNode(m_friendsList[i].client_list, MAX_FRIEND_CLIENTS, client_id, ip_port))
			{
				//if we can't replace bad nodes we try replacing good ones
				replaceGoodNode(m_friendsList[i].client_list, MAX_FRIEND_CLIENTS, client_id, ip_port,m_selfClientID.data());
			}
		}
	}
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
		if ( memcmp(list[i].client_id, clientID.data(), CLIENT_ID_SIZE) == 0)
			return true;	
	}	
	return false;
}

bool CDistributedHashTable::clientInList(ClientData* list, uint32_t length, char* client_id, IP_Port ip_port)
{	
	for (int i = 0; i < length; i++)
	{
		//If the id for an ip/port changes, replace it.
		if (list[i].ip_port.ip == ip_port.ip &&	list[i].ip_port.port == ip_port.port)	
			memcpy(list[i].client_id, client_id, CLIENT_ID_SIZE);
		
		if(memcmp(list[i].client_id,client_id, CLIENT_ID_SIZE)==0)
		{
			//Refresh the client timestamp.
			list[i].timestamp = CUtil::currentTime();
			return true;
		}
	}	
	return false;
}

bool CDistributedHashTable::replaceBadNode(ClientData* list, uint32_t length, char* client_id, IP_Port ip_port)
{	
	uint32_t temp_time = CUtil::currentTime();
	for (int i = 0; i < length; i++)
	{
		if (temp_time -list[i].timestamp > BAD_NODE_TIMEOUT *1000)//if node is bad.
		{
			memcpy(list[i].client_id, client_id, CLIENT_ID_SIZE);
			list[i].ip_port = ip_port;
			list[i].timestamp = temp_time;
			return true;
		}
	}
	return false;
}

bool CDistributedHashTable::replaceGoodNode(ClientData* list, uint32_t length, char* client_id, IP_Port ip_port, char* comp_client_id)
{
	for (int i = 0; i < length; i++)
	{
		if ( idClosest(comp_client_id, list[i].client_id, client_id) == 2)
		{
			memcpy(list[i].client_id, client_id, CLIENT_ID_SIZE);
			list[i].ip_port = ip_port;
			list[i].timestamp = CUtil::currentTime();
			return true;
		}
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
			onPingRequest(packet, source);
		case 1:
			onPingResponses(packet,  source);
		case 2:
			onNodesRequest(packet, source);
		case 3:
			onNodesResponses(packet, source);
		default:
			return false;
	}
	return true;
}

bool CDistributedHashTable::isPinging(IP_Port ipport, uint32_t pingID)
{
	auto tempTime = CUtil::currentTime();
	for (auto& pings : m_pingsList)
	{
		if (tempTime - pings.timestamp < PING_TIMEOUT * 1000)
		{
			if (!ipport.ip.isNull() && pings.ip_port.ip == ipport.ip && pings.ip_port.port == ipport.port)
				return true;

			if (pingID != 0&&pings.ping_id==pingID)
				return true;
		}
	}
	return false;
}


