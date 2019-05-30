#include "CUtil.h"
#include <qdatetime.h>
#include <random>
#include <qnetworkaccessmanager.h>
#include <qnetworkreply.h>
#include <qnetworkrequest.h>
#include <qjsondocument.h>
#include <qdebug.h>
#include <qeventloop.h>
#include <qjsonobject.h>
#include <QRegularExpression>
#include <QJsonArray>


const QUrl NodeListAddress{ "https://nodes.tox.chat/json" };
const QLatin1Literal jsonNodeArrayName{ "nodes" };
const QLatin1Literal emptyAddress{ "-" };
const QRegularExpression ToxPkRegEx(QString("(^|\\s)[A-Fa-f0-9]{%1}($|\\s)").arg(64));
const QLatin1Literal builtinNodesFile{ ":/conf/nodes.json" };
namespace NodeFields {
	const QLatin1Literal status_udp{ "status_udp" };
	const QLatin1Literal status_tcp{ "status_tcp" };
	const QLatin1Literal ipv4{ "ipv4" };
	const QLatin1Literal ipv6{ "ipv6" };
	const QLatin1Literal public_key{ "public_key" };
	const QLatin1Literal port{ "port" };
	const QLatin1Literal maintainer{ "maintainer" };
	// TODO(sudden6): make use of this field once we differentiate between TCP nodes, and bootstrap nodes
	const QLatin1Literal tcp_ports{ "tcp_ports" };
	const QStringList neededFields{ status_udp, status_tcp, ipv4, ipv6, public_key, port, maintainer };
} // namespace NodeFields





CUtil::CUtil()
{
}


CUtil::~CUtil()
{
}

int64_t CUtil::currentTime()
{
	return QDateTime::currentDateTime().toMSecsSinceEpoch();
}


int CUtil::randomInt(int n)
{
	std::default_random_engine randEngine((std::random_device())());
	if (n == 0)
	{
		std::uniform_int_distribution<int> distribution;
		return distribution(randEngine);
	}		
	else
	{
		std::uniform_int_distribution<int> distribution(0, n);
		return distribution(randEngine);
	}	
}

bool CUtil::loadDefaultBootstrapNodes(QList<DhtServer>& list)
{
	QNetworkAccessManager manager;
	QEventLoop loop;
	auto ret = manager.get(QNetworkRequest(NodeListAddress));
	QObject::connect(ret, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	loop.exec();
	auto josnData = ret->readAll();
	qDebug() << josnData;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(josnData);
	if (jsonDoc.isNull()) {
		qWarning() << "Failed to parse JSON document";
		return false;
	}
	QJsonObject rootObj = jsonDoc.object();
	if (!(rootObj.contains(jsonNodeArrayName) && rootObj[jsonNodeArrayName].isArray())) {
		return false;
	}
	QJsonArray nodes = rootObj[jsonNodeArrayName].toArray();
	for (const auto& node : nodes)
	{
		if (node.isObject())
		{
			jsonNodeToDhtServer(node.toObject(), list);
		}
	}
	return true;
}

void CUtil::jsonNodeToDhtServer(const QJsonObject& node, QList<DhtServer>& outList)
{
	// first check if the node in question has all needed fields
	bool found = true;
	for (const auto& key : NodeFields::neededFields) {
		found |= node.contains(key);
	}
	if (!found) {
		return;
	}
	// only use nodes that provide at least UDP connection
	if (!node[NodeFields::status_udp].toBool(false)) {
		return;
	}
	const QString public_key = node[NodeFields::public_key].toString({});
	const int port = node[NodeFields::port].toInt(-1);
	// nodes.tox.chat doesn't use empty strings for empty addresses
	QString ipv6_address = node[NodeFields::ipv6].toString({});
	if (ipv6_address == emptyAddress) {
		ipv6_address = QString{};
	}
	QString ipv4_address = node[NodeFields::ipv4].toString({});
	if (ipv4_address == emptyAddress) {
		ipv4_address = QString{};
	}
	const QString maintainer = node[NodeFields::maintainer].toString({});

	if (port < 1 || port > std::numeric_limits<uint16_t>::max()) {
		return;
	}
	const quint16 port_u16 = static_cast<quint16>(port);
	if (!public_key.contains(ToxPkRegEx)) {
		return;
	}
	DhtServer server;
	server.userId = public_key;
	server.port = port_u16;
	server.name = maintainer;

	if (!ipv4_address.isEmpty()) {
		server.address = ipv4_address;
		outList.append(server);
	}
	// avoid adding the same server twice in case they use the same dns name for v6 and v4
	if (!ipv6_address.isEmpty() && ipv4_address != ipv6_address) {
		server.address = ipv6_address;
		outList.append(server);
	}
	return;
}

