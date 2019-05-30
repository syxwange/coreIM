#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_mainwindow.h"

class CCoreIM;
class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = Q_NULLPTR);

private:
	Ui::MainWindowClass ui;
	CCoreIM* m_pCore{ nullptr };
};
