#pragma once
#include <qstring.h>


struct DhtServer;
class QJsonObject;

struct DhtServer
{
	QString name;
	QString userId;
	QString address;
	quint16 port;

	bool operator==(const DhtServer& other) const {
		return this == &other || (port == other.port && address == other.address
			&& userId == other.userId && name == other.name);
	}
	bool operator!=(const DhtServer& other) const { return !(*this == other); }
};

class CUtil
{
public:
	CUtil();
	~CUtil();

	//���غ��뼶ʱ���
	static long long  currentTime();

	//�����������
	static int randomInt(int n=0);

	static bool  loadDefaultBootstrapNodes(QList<DhtServer> & list);

	static void jsonNodeToDhtServer(const QJsonObject& node, QList<DhtServer>& outList);
};

