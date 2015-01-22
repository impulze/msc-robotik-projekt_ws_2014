#include "drawing.h"
#include "drawwidget.h"
#include "stats.h"
#include "widgets.h"

#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtGui/QIntValidator>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTextEdit>

#include <vector>

#include <assert.h>

namespace
{
	QString secondsString(uint64_t msec);

	QString secondsString(uint64_t msec)
	{
#if 0
		if (nsec > 60 * 1000 * 1000) {
			uint64_t mins = nsec / (60 * 1000 * 1000);
			uint64_t _nsec = nsec - mins * (60 * 1000 * 1000);
			uint64_t secs = _nsec / (1000 * 1000);
			_nsec = _nsec - secs * (1000 * 1000);
			uint64_t msec = _nsec / 1000;
			_nsec = _nsec - msec * 1000;
			return QString::number(mins) + " m " + QString::number(secs) + " s " + QString::number(msec) + " ms " + QString::number(_nsec) + " ns";
		} else if (nsec > 1000 * 1000) {
			uint64_t secs = nsec / (1000 * 1000);
			uint64_t _nsec = nsec - secs * (1000 * 1000);
			uint64_t msec = _nsec / 1000;
			_nsec = _nsec - msec * 1000;
			return QString::number(secs) + " s " + QString::number(msec) + " ms " + QString::number(_nsec) + " ns";
		} else if (nsec > 1000) {
			uint64_t msec = nsec / 1000;
			uint64_t _nsec = nsec % 1000;
			return QString::number(msec) + " ms " + QString::number(_nsec) + " ns";
		} else {
			return QString::number(nsec) + " ns";
		}
#else
		if (msec > 60 * 1000) {
			uint64_t mins = msec / (60 * 1000);
			uint64_t _msec = msec - mins * (60 * 1000);
			uint64_t secs = _msec / 1000;
			_msec = _msec - secs * 1000;
			return QString::number(mins) + " m " + QString::number(secs) + " s " + QString::number(_msec) + " ms";
		} else if (msec > 1000) {
			uint64_t secs = msec / 1000;
			uint64_t _msec = msec % 1000;
			return QString::number(secs) + " s " + QString::number(_msec) + " ms";
		} else {
			return QString::number(msec) + " ms";
		}
#endif
	}
}

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	CentralWidget *central = new CentralWidget(this);

	QMenu *menuRoom = menuBar()->addMenu(tr("&Project"));
	QAction *actRoomLoad = menuRoom->addAction(tr("&Load room"));
	connect(actRoomLoad, SIGNAL(triggered()), central, SLOT(wantsRoomLoaded()));
	QAction *actProjectLoad = menuRoom->addAction(tr("&Load project"));
	connect(actProjectLoad, SIGNAL(triggered()), central, SLOT(wantsProjectLoaded()));
	QAction *actProjectSave = menuRoom->addAction(tr("&Save project"));
	connect(actProjectSave, SIGNAL(triggered()), central, SLOT(wantsProjectSaved()));

	QAction *actQuit = menuBar()->addAction(tr("&Quit"));
	connect(actQuit, SIGNAL(triggered()), this, SLOT(close()));

	setWindowTitle("Pathfinding in 2D rooms");
	layout()->setContentsMargins(0, 0, 0, 0);

	setCentralWidget(central);

	resize(1024, 768);
}

CentralWidget::CentralWidget(QWidget *parent)
	: QWidget(parent),
	  drawing_(0),
	  drawWidget_(0),
	  stats_(0)
{
	QLabel *statusLabel = new QLabel(tr("Status"));
	statusText_ = new QTextEdit(this);
	statusText_->setDisabled(true);
	QLabel *helpLabel = new QLabel(tr("Help"));
	helpText_ = new QTextEdit(this);
	helpText_->setDisabled(true);
	QLabel *algorithmsLabel = new QLabel(tr("Algorithms"));

	QLabel *amountLabel = new QLabel(tr("Amount of nodes"), this);
	amountField_ = new QLineEdit(this);
	amountField_->setValidator(new QIntValidator(0, 20000, this));
	connect(amountField_, SIGNAL(returnPressed()), this, SLOT(amountOfNodesChanged()));

	boxAdd_ = new QCheckBox(tr("Add waypoint"), this);
	boxDel_ = new QCheckBox(tr("Remove waypoint"), this);
	boxStart_ = new QCheckBox(tr("Set startpoint"), this);
	boxEnd_ = new QCheckBox(tr("Set endpoint"), this);
	boxShowTri_ = new QCheckBox(tr("Show triangulation"), this);
	boxShowRoomTri_ = new QCheckBox(tr("Show room triangulation"), this);
	boxShowWay_ = new QCheckBox(tr("Show waypoints"), this);
	boxShowPath_ = new QCheckBox(tr("Show path"), this);
	boxShowNeighbours_ = new QCheckBox(tr("Show neighbours"), this);
	boxAlgorithms_ = new QComboBox(this);
	boxAlgorithms_->addItem("Dijkstra");
	boxAlgorithms_->addItem("A*");
	buttonAnimate_ = new QPushButton(tr("Animate"), this);
	buttonStats_ = new QPushButton(tr("Statistics"), this);

	connect(boxAdd_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxDel_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxStart_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxEnd_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxShowTri_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxShowRoomTri_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxShowWay_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxShowPath_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxShowNeighbours_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxAlgorithms_, SIGNAL(activated(int)), this, SLOT(checkBoxChanged(int)));
	connect(buttonAnimate_, SIGNAL(clicked()), this, SLOT(buttonClicked()));
	connect(buttonStats_, SIGNAL(clicked()), this, SLOT(buttonClicked()));

	QVBoxLayout *sideLayout = new QVBoxLayout;
	QLayout *amountLayout = new QHBoxLayout;
	amountLayout->addWidget(amountLabel);
	amountLayout->addWidget(amountField_);
	sideLayout->addLayout(amountLayout);
	sideLayout->addWidget(boxAdd_);
	sideLayout->addWidget(boxDel_);
	sideLayout->addWidget(boxStart_);
	sideLayout->addWidget(boxEnd_);
	QFrame *line1 = new QFrame(this);
	line1->setFrameShape(QFrame::HLine);
	line1->setFrameShadow(QFrame::Sunken);
	sideLayout->addWidget(line1);
	sideLayout->addWidget(boxShowTri_);
	sideLayout->addWidget(boxShowRoomTri_);
	sideLayout->addWidget(boxShowWay_);
	sideLayout->addWidget(boxShowPath_);
	sideLayout->addWidget(boxShowNeighbours_);
	sideLayout->addWidget(buttonAnimate_);
	sideLayout->addWidget(buttonStats_);
	QFrame *line2 = new QFrame(this);
	line2->setFrameShape(QFrame::HLine);
	line2->setFrameShadow(QFrame::Sunken);
	sideLayout->addWidget(line2);
	sideLayout->addWidget(algorithmsLabel);
	sideLayout->addWidget(boxAlgorithms_);
	QFrame *line3 = new QFrame(this);
	line3->setFrameShape(QFrame::HLine);
	line3->setFrameShadow(QFrame::Sunken);
	sideLayout->addWidget(line3);
	QSpacerItem *spacer2 = new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	sideLayout->addItem(spacer2);
	sideLayout->addWidget(statusLabel);
	sideLayout->addWidget(statusText_);
	QFrame *line4 = new QFrame(this);
	line4->setFrameShape(QFrame::HLine);
	line4->setFrameShadow(QFrame::Sunken);
	sideLayout->addWidget(line4);
	sideLayout->addWidget(helpLabel);
	sideLayout->addWidget(helpText_);
	QHBoxLayout *mainLayout = new QHBoxLayout(this);
	mainLayout->addLayout(sideLayout);

	CheckBoxEventFilter *filter = new CheckBoxEventFilter(this);
	boxAdd_->installEventFilter(filter);
	boxDel_->installEventFilter(filter);
	boxStart_->installEventFilter(filter);
	boxEnd_->installEventFilter(filter);
	boxShowTri_->installEventFilter(filter);
	boxShowRoomTri_->installEventFilter(filter);
	boxShowWay_->installEventFilter(filter);
	boxShowPath_->installEventFilter(filter);
	boxShowNeighbours_->installEventFilter(filter);
}

CentralWidget::~CentralWidget()
{
	delete drawWidget_;
	delete drawing_;
	delete stats_;
}

void CentralWidget::wantsRoomLoaded()
{
	QString filename = QFileDialog::getOpenFileName(this, tr("Open room image"), "", "Images (*.png)");

	if (filename.length() == 0) {
		return;
	}

	removeRoom();

	createNewDrawing();

	drawing_->loadRoom(filename.toStdString().c_str());

	createDrawWidget();
}

void CentralWidget::wantsProjectLoaded()
{
	QString filename = QFileDialog::getOpenFileName(this, tr("Load project"), "", "XML files (*.xml)");

	if (filename.length() == 0) {
		return;
	}

	removeRoom();

	QFile loadFile(filename);
	loadFile.open(QIODevice::ReadOnly | QIODevice::Text);

	QXmlStreamReader reader(&loadFile);

	createNewDrawing();

	QString errorString;

	if (reader.readNextStartElement()) {
		QString name = reader.name().toString();

		if (name != "project") {
			errorString = "Expected <project> where <" + name + "> is.";
		}
	} else {
		errorString = reader.errorString();
	}

	if (errorString.length() == 0) {
		reader.readNextStartElement();

		if (!drawing_->loadProject(&reader)) {
			errorString = "The drawing element could not be read properly.";
		}
	}

	if (errorString.length() == 0) {
		reader.readNext();

		if (reader.isCharacters()) {
			reader.readNext();
		}

		if (!reader.isEndElement()) {
			errorString = "Expected the end element for <project> where " + reader.tokenString() + " is.";
		} else {
			QString name = reader.name().toString();

			if (name != "project") {
				errorString = "Expected </project> where <" + name + "> is.";
			}
		}
	}

	if (errorString.length() != 0) {
		QDialog *errorDialog(new QDialog(this));
		errorDialog->setWindowTitle("XML errors while reading XML project");
		QVBoxLayout *layout = new QVBoxLayout(errorDialog);
		QTextEdit *textEdit = new QTextEdit(this);

		textEdit->insertPlainText("The following error occured:\n");
		textEdit->insertPlainText(errorString);

		QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, Qt::Horizontal, errorDialog);

		connect(buttons, SIGNAL(rejected()), errorDialog, SLOT(reject()));

		layout->addWidget(textEdit);
		layout->addWidget(buttons);

		errorDialog->exec();
		delete errorDialog;

		delete drawing_;
		drawing_ = 0;
	} else {
		createDrawWidget();
	}
}

void CentralWidget::wantsProjectSaved()
{
	QString filename = QFileDialog::getSaveFileName(this, tr("Save project"), "", "XML files (*.xml)");

	if (filename.length() == 0) {
		return;
	}

	std::string error;

	if (drawing_) {
		QFile saveFile(filename);
		saveFile.open(QIODevice::WriteOnly | QIODevice::Text);

		QXmlStreamWriter writer(&saveFile);
		writer.writeStartDocument();
		writer.setAutoFormatting(true);
		writer.writeStartElement("", "project");

		if (!drawing_->saveProject(&writer)) {
			error = "The XML stream writer could not create output.";
		}

		writer.writeEndElement();
		writer.writeEndDocument();
	} else {
		error = "No room image loaded yet.";
	}

	if (!error.empty()) {
		QDialog *errorDialog(new QDialog(this));
		errorDialog->setWindowTitle("XML errors while writing XML project");
		QVBoxLayout *layout = new QVBoxLayout(errorDialog);
		QTextEdit *textEdit = new QTextEdit(this);

		textEdit->insertPlainText("The following error occured:\n");
		textEdit->insertPlainText(QString::fromStdString(error));

		QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, Qt::Horizontal, errorDialog);

		layout->addWidget(textEdit);
		layout->addWidget(buttons);

		errorDialog->exec();
		delete errorDialog;
	}
}


void CentralWidget::checkBoxChanged(int state)
{
	if (!drawing_) {
		return;
	}

	QObject *sender = QObject::sender();

	if (sender == boxAdd_ || sender == boxDel_ ||
	    sender == boxStart_ || sender == boxEnd_) {
		if (state != Qt::Checked) {
			drawing_->setWaypointModification(Drawing::WaypointNoMod);
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
	} else if (sender == boxAlgorithms_) {
		if (state == 0) {
			drawing_->setAlgorithm(Room::Dijkstra);
		} else {
			drawing_->setAlgorithm(Room::AStar);
		}
		return;
	}

	Drawing::Option option;

	if (sender == boxShowTri_) {
		option = Drawing::ShowTriangulation;
	} else if (sender == boxShowRoomTri_) {
		option = Drawing::ShowRoomTriangulation;
	} else if (sender == boxShowWay_) {
		option = Drawing::ShowWaypoints;
	} else if (sender == boxShowPath_) {
		option = Drawing::ShowPath;
	} else {
		option = Drawing::ShowNeighbours;
	}

	drawing_->setOption(option, state == Qt::Checked);
}

void CentralWidget::buttonClicked()
{
	if (!drawing_) {
		return;
	}

	QObject *sender = QObject::sender();

	if (sender == buttonAnimate_) {
		drawing_->animate();
	} else if (sender == buttonStats_) {
		QDialog *statsDialog(new QDialog(this));
		statsDialog->setWindowTitle("Statistics");
		QVBoxLayout *layout = new QVBoxLayout(statsDialog);
		QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, statsDialog);

		connect(buttons, SIGNAL(accepted()), statsDialog, SLOT(accept()));

		QTableWidget *table = new QTableWidget(statsDialog);
		table->verticalHeader()->hide();
		table->horizontalHeader()->hide();
		table->setRowCount(6);
		table->setColumnCount(2);

		unsigned int width = 0;
		unsigned int height = 0;

		QTableWidgetItem *item;

		item = new QTableWidgetItem(tr("Last path calculation:"));
		table->setItem(0, 0, item);

		item = new QTableWidgetItem(secondsString(stats_->lastPathCalculation));
		table->setItem(0, 1, item);

		item = new QTableWidgetItem(tr("Last path collision calculation:"));
		table->setItem(1, 0, item);

		item = new QTableWidgetItem(secondsString(stats_->lastPathCollisionCalculation));
		table->setItem(1, 1, item);

		item = new QTableWidgetItem(tr("Last Catmull Rom calculation:"));
		table->setItem(2, 0, item);

		item = new QTableWidgetItem(secondsString(stats_->lastCatmullRomCalculation));
		table->setItem(2, 1, item);

		item = new QTableWidgetItem(tr("Last random placement of waypoints:"));
		table->setItem(3, 0, item);

		item = new QTableWidgetItem(secondsString(stats_->lastSetNodes));
		table->setItem(3, 1, item);

		item = new QTableWidgetItem(tr("Last room triangulation calculation:"));
		table->setItem(4, 0, item);

		item = new QTableWidgetItem(secondsString(stats_->lastRoomTriangulationCalculation));
		table->setItem(4, 1, item);

		item = new QTableWidgetItem(tr("Last used algorithm:"));
		table->setItem(5, 0, item);

		item = new QTableWidgetItem(stats_->lastUsedAlgorithm == Room::Dijkstra ? "Dijkstra" : "A*");
		table->setItem(5, 1, item);

		table->setEditTriggers(QAbstractItemView::NoEditTriggers);
		table->resizeRowsToContents();
		table->resizeColumnsToContents();

		for (int i = 0; i < table->rowCount(); i++) {
			height += table->rowHeight(i);
		}

		for (int j = 0; j < table->columnCount(); j++) {
			width += table->columnWidth(j);
		}

		layout->addWidget(table);
		layout->addWidget(buttons);

		statsDialog->setMinimumSize(width + 30, height + 120);
		statsDialog->exec();
		delete statsDialog;

	}
}

void CentralWidget::amountOfNodesChanged()
{
	if (!drawing_) {
		return;
	}

	QLineEdit *lineEdit = static_cast<QLineEdit *>(sender());
	int num = lineEdit->text().toInt();

	drawing_->setNodes(num);
	amountField_->setText(QString::number(drawing_->countWaypoints()));
}

void CentralWidget::sceneCouldChange()
{
	amountField_->setText(QString::number(drawing_->countWaypoints()));
}

void CentralWidget::removeRoom()
{
	if (drawWidget_) {
		QWidget *widget = layout()->takeAt(layout()->indexOf(drawWidget_))->widget();
		DrawWidget *drawWidget = static_cast<DrawWidget *>(widget);
		layout()->removeWidget(drawWidget);
	}

	delete drawWidget_;
	drawWidget_ = 0;

	delete drawing_;
	drawing_ = 0;
}

void CentralWidget::createDrawWidget()
{
	drawWidget_ = new DrawWidget(drawing_, this);
	amountField_->setText(QString::number(drawing_->countWaypoints()));
	connect(drawWidget_, SIGNAL(mouseClicked()), this, SLOT(sceneCouldChange()));
	static_cast<QHBoxLayout *>(layout())->addWidget(drawWidget_, 1);

	boxAdd_->setCheckState(Qt::Unchecked);
	boxDel_->setCheckState(Qt::Unchecked);
	boxStart_->setCheckState(Qt::Unchecked);
	boxEnd_->setCheckState(Qt::Unchecked);

	boxShowTri_->setCheckState(drawing_->getOption(Drawing::ShowTriangulation) ? Qt::Checked : Qt::Unchecked);
	boxShowRoomTri_->setCheckState(drawing_->getOption(Drawing::ShowRoomTriangulation) ? Qt::Checked : Qt::Unchecked);
	boxShowWay_->setCheckState(drawing_->getOption(Drawing::ShowWaypoints) ? Qt::Checked : Qt::Unchecked);
	boxShowPath_->setCheckState(drawing_->getOption(Drawing::ShowPath) ? Qt::Checked : Qt::Unchecked);
	boxShowNeighbours_->setCheckState(drawing_->getOption(Drawing::ShowNeighbours) ? Qt::Checked : Qt::Unchecked);
	boxAlgorithms_->setCurrentIndex(drawing_->getAlgorithm() == Room::Dijkstra ? 0 : 1);
}

void CentralWidget::createNewDrawing()
{
	delete stats_;
	stats_ = new Stats();

	drawing_ = new Drawing(stats_, statusText_, helpText_);
}

bool CentralWidget::checkBoxEvent(QObject *object, QEvent *event)
{
	if (event->type() != QEvent::Enter && event->type() != QEvent::Leave) {
		return QObject::eventFilter(object, event);
	}

	QObject *sender = object;
	std::string showText;

	if (sender == boxAdd_) {
		showText = "Add waypoints (left mouse click) if it's inside the room domain.";
	} else if (sender == boxDel_) {
		showText = "Delete a waypoint (left mouse click on a waypoint).";
	} else if (sender == boxStart_) {
		showText = "Set the startpoint (left mouse click) if it's inside the room domain.";
	} else if (sender == boxEnd_) {
		showText = "Set the endpoint (left mouse click) if it's inside the room domain.";
	} else if (sender == boxShowTri_) {
		showText = "Show the triangulation of all waypoints (including startpoint and endpoint).";
	} else if (sender == boxShowRoomTri_) {
		showText = "Show the room triangulation of all corner vertices of the room. This triangulation is used to check if a point is inside the domain (triangles).";
	} else if (sender == boxShowWay_) {
		showText = "Show the waypoints (including startpoint and endpoint).";
	} else if (sender == boxShowPath_) {
		showText = "Show the generated path (if possible) and collisions with red markers (if any).";
	} else if (sender == boxShowNeighbours_) {
		showText = "Show the neighbours of a waypoint (or startpoint and endpoint). Click on a point and green edges are reachable while red ones are not.";
	}

	if (!showText.empty()) {
		if (event->type() == QEvent::Leave) {
			showText = "";
		}

		helpText_->setText(QString::fromStdString(showText));
	}

	return QObject::eventFilter(object, event);
}

CheckBoxEventFilter::CheckBoxEventFilter(CentralWidget *central)
	: QObject(central),
	  central_(central)
{
}

bool CheckBoxEventFilter::eventFilter(QObject *object, QEvent *event)
{
	return central_->checkBoxEvent(object, event);
}
