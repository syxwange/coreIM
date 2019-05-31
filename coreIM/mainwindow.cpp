#include "mainwindow.h"
#include "CUtil.h"
#include <qdebug.h>
#include "CCoreIM.h"
#include <qpushbutton.h>

MainWindow::MainWindow(QWidget *parent)	: QMainWindow(parent)
{
	ui.setupUi(this);
	m_pCore = CCoreIM::getInstance();	
	connect(ui.btnAddFriend, &QPushButton::clicked, this, &MainWindow::onBtnAddFriend);
	connect(this, &MainWindow::sendBootstrap, m_pCore, &CCoreIM::bootstrap);
	connect(this, &MainWindow::sendAddFriend, m_pCore, &CCoreIM::addFriend);	
	
	m_pCore->initNetwork(QHostAddress::LocalHost);
	IP_Port ipport{ QHostAddress("127.0.0.1"),33445,0 };
	m_pCore->bootstrap(ipport);
	m_pCore->start();

}

void MainWindow::onBtnAddFriend()
{
	auto clientID = ui.lineEditFriend->text().toUtf8();
	

	emit sendAddFriend(clientID);
}
