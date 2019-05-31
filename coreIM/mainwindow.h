#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_mainwindow.h"
#include "CCoreIM.h"

class CCoreIM;
class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = Q_NULLPTR);

	void onBtnAddFriend();

	

signals:
	//

	void sendBootstrap(IP_Port);

	void sendAddFriend(QByteArray);

private:
	Ui::MainWindowClass ui;
	CCoreIM* m_pCore{ nullptr };
};
