#include "mainwindow.h"
#include "CUtil.h"
#include <qdebug.h>
#include "CCoreIM.h"

MainWindow::MainWindow(QWidget *parent)	: QMainWindow(parent)
{
	m_pCore = CCoreIM::getInstance();
	
	m_pCore->start();
	ui.setupUi(this);
	
}
