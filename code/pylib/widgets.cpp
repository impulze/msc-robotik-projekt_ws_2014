#include "drawing.h"
#include "drawwidget.h"
#include "widgets.h"

#include <QtGui/QIntValidator>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenuBar>

#include <cstdio>
#include <vector>

#include <assert.h>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	QMenu *menuRoom = menuBar()->addMenu(tr("&Room"));
	QAction *actRoomNew = menuRoom->addAction(tr("&New"));
	static_cast<void>(actRoomNew);
	QAction *actRoomLoad = menuRoom->addAction(tr("&Load"));
	connect(actRoomLoad, SIGNAL(triggered()), this, SLOT(wantsRoomLoaded()));
	QAction *actRoomSave = menuRoom->addAction(tr("&Save"));
	connect(actRoomSave, SIGNAL(triggered()), this, SLOT(wantsRoomSaved()));

	QMenu *menuRobot = menuBar()->addMenu(tr("R&obot"));
	QAction *actRobotSimulate = menuRobot->addAction(tr("&Simulate"));
	static_cast<void>(actRobotSimulate);
	QAction *actRobotExport = menuRobot->addAction(tr("&Export Simulation"));
	static_cast<void>(actRobotExport);

	QAction *actQuit = menuBar()->addAction(tr("&Quit"));
	connect(actQuit, SIGNAL(triggered()), this, SLOT(close()));

	setWindowTitle("Simulation");
	layout()->setContentsMargins(0, 0, 0, 0);

	setCentralWidget(new CentralWidget(this));

	resize(1024, 768);
}

void MainWindow::wantsRoomLoaded()
{
	QString filename = QFileDialog::getOpenFileName(this, tr("Open room image"), "", "Images (*.png)");

	if (filename.length() == 0) {
		return;
	}

	std::printf("Want to open: %s\n", filename.toStdString().c_str());
}

void MainWindow::wantsRoomSaved()
{
	QString filename = QFileDialog::getSaveFileName(this, tr("Save room image"), "", "Images (*.png)");

	if (filename.length() == 0) {
		return;
	}

	std::printf("Want to save: %s\n", filename.toStdString().c_str());
}

CentralWidget::CentralWidget(QWidget *parent)
	: QWidget(parent)
{
	drawing_ = new Drawing;
	drawWidget_ = new DrawWidget(drawing_, this);

	QLabel *amountLabel = new QLabel(tr("Amount of nodes"), this);
	QLineEdit *amountField = new QLineEdit(this);
	amountField->setValidator(new QIntValidator(2, 20000, this));
	connect(amountField, SIGNAL(returnPressed()), this, SLOT(amountOfNodesChanged()));

	boxAdd_ = new QCheckBox(tr("Add waypoint"), this);
	boxDel_ = new QCheckBox(tr("Remove waypoint"), this);
	boxStart_ = new QCheckBox(tr("Set startpoint"), this);
	boxEnd_ = new QCheckBox(tr("Set endpoint"), this);
	boxShowTri_ = new QCheckBox(tr("Show triangulation"), this);
	boxShowRoomTri_ = new QCheckBox(tr("Show room triangulation"), this);
	boxShowWay_ = new QCheckBox(tr("Show waypoints"), this);
	boxShowPath_ = new QCheckBox(tr("Show path"), this);

	connect(boxAdd_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxDel_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxStart_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxEnd_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxShowTri_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxShowRoomTri_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxShowWay_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxShowPath_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));

	QVBoxLayout *sideLayout = new QVBoxLayout;
	QLayout *amountLayout = new QHBoxLayout;
	amountLayout->addWidget(amountLabel);
	amountLayout->addWidget(amountField);
	sideLayout->addLayout(amountLayout);
	sideLayout->addWidget(boxAdd_);
	sideLayout->addWidget(boxDel_);
	sideLayout->addWidget(boxStart_);
	sideLayout->addWidget(boxEnd_);
	QFrame *line = new QFrame(this);
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);
	sideLayout->addWidget(line);
	sideLayout->addWidget(boxShowTri_);
	sideLayout->addWidget(boxShowRoomTri_);
	sideLayout->addWidget(boxShowWay_);
	sideLayout->addWidget(boxShowPath_);
	QSpacerItem *spacer = new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	sideLayout->addItem(spacer);
	QHBoxLayout *mainLayout = new QHBoxLayout(this);
	mainLayout->addLayout(sideLayout);
	mainLayout->addWidget(drawWidget_, 1);

	boxShowWay_->setCheckState(Qt::Checked);
	boxShowPath_->setCheckState(Qt::Checked);
}

void CentralWidget::checkBoxChanged(int state)
{
	QObject *sender = QObject::sender();

	if (sender == boxAdd_ || sender == boxDel_ ||
	    sender == boxStart_ || sender == boxEnd_) {
		if (state != Qt::Checked) {
			return;
		}

		std::vector<QCheckBox *> exclusives;

		exclusives.push_back(boxAdd_);
		exclusives.push_back(boxDel_);
		exclusives.push_back(boxStart_);
		exclusives.push_back(boxEnd_);

		std::vector<QCheckBox *>::iterator thisCheckBox = std::find(exclusives.begin(), exclusives.end(), sender);

		assert(thisCheckBox != exclusives.end());

		exclusives.erase(thisCheckBox);

		for (std::vector<QCheckBox *>::iterator it = exclusives.begin();
		     it != exclusives.end();
		     ++it) {
			if ((*it)->checkState() == Qt::Checked) {
				(*it)->setCheckState(Qt::Unchecked);
			}
		}

		Drawing::WaypointModification mod;

		if (sender == boxAdd_) {
			mod = Drawing::WaypointAdd;
		} else if (sender == boxDel_) {
			mod = Drawing::WaypointDelete;
		} else if (sender == boxStart_) {
			mod = Drawing::WaypointStart;
		} else {
			mod = Drawing::WaypointEnd;
		}

		drawing_->setWaypointModification(mod);
		return;
	}

	Drawing::Option option;

	if (sender == boxShowTri_) {
		option = Drawing::ShowTriangulation;
	} else if (sender == boxShowRoomTri_) {
		option = Drawing::ShowRoomTriangulation;
	} else if (sender == boxShowWay_) {
		option = Drawing::ShowWaypoints;
	} else {
		option = Drawing::ShowPath;
	}

	drawing_->setOption(option, state == Qt::Checked);
}

void CentralWidget::amountOfNodesChanged()
{
	QLineEdit *lineEdit = static_cast<QLineEdit *>(sender());
	int num = lineEdit->text().toInt();

	std::printf("The amount of nodes changed to: '%d'\n", num);

	drawing_->setNodes(num);
}
