#include "CCoreIM.h"
#include <qthread.h>
#include <qtimer.h>

CCoreIM* CCoreIM::m_pCoreIM{ nullptr };

CCoreIM* CCoreIM::getInstance()
{
	if (!m_pCoreIM)
	{
		m_pCoreIM = new CCoreIM;
	}
	return m_pCoreIM;
}

CCoreIM::CCoreIM(QObject *parent)	: QObject(parent),m_pThread(new QThread),m_pTimer(new QTimer(this))
{
	moveToThread(m_pThread);
	connect(m_pThread, &QThread::started, this, &CCoreIM::onStarted);
}

void CCoreIM::onStarted()
{
}

CCoreIM::~CCoreIM()
{
}
