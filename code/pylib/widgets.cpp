#include "drawing.h"
#include "drawwidget.h"
#include "widgets.h"

#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtGui/QIntValidator>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>

#include <vector>

#include <assert.h>

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
	  drawWidget_(0)
{
	QLabel *statusLabel = new QLabel(tr("Status"));
	statusText_ = new QTextEdit(this);
	statusText_->setDisabled(true);
	QLabel *helpLabel = new QLabel(tr("Help"));
	helpText_ = new QTextEdit(this);
	helpText_->setDisabled(true);

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
	buttonAnimate_ = new QPushButton(tr("Animate"), this);

	connect(boxAdd_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxDel_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxStart_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxEnd_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxShowTri_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxShowRoomTri_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxShowWay_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxShowPath_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(boxShowNeighbours_, SIGNAL(stateChanged(int)), this, SLOT(checkBoxChanged(int)));
	connect(buttonAnimate_, SIGNAL(clicked()), this, SLOT(buttonClicked()));

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
	QFrame *line2 = new QFrame(this);
	line2->setFrameShape(QFrame::HLine);
	line2->setFrameShadow(QFrame::Sunken);
	sideLayout->addWidget(line2);
	QSpacerItem *spacer = new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	sideLayout->addItem(spacer);
	sideLayout->addWidget(statusLabel);
	sideLayout->addWidget(statusText_);
	QFrame *line3 = new QFrame(this);
	line3->setFrameShape(QFrame::HLine);
	line3->setFrameShadow(QFrame::Sunken);
	sideLayout->addWidget(line3);
	sideLayout->addWidget(helpLabel);
	sideLayout->addWidget(helpText_);
	QHBoxLayout *mainLayout = new QHBoxLayout(this);
	mainLayout->addLayout(sideLayout);

	boxShowWay_->setCheckState(Qt::Checked);
	showOptions_[Drawing::ShowWaypoints] = true;
	boxShowPath_->setCheckState(Qt::Checked);
	showOptions_[Drawing::ShowPath] = true;

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
}

void CentralWidget::wantsRoomLoaded()
{
	QString filename = QFileDialog::getOpenFileName(this, tr("Open room image"), "", "Images (*.png)");

	if (filename.length() == 0) {
		return;
	}

	removeRoom();

	drawing_ = new Drawing(statusText_, helpText_);
	drawing_->fromImage(filename.toStdString().c_str());
	drawWidget_ = new DrawWidget(drawing_, this);
	amountField_->setText(QString::number(drawing_->countWaypoints()));
	connect(drawWidget_, SIGNAL(mouseClicked()), this, SLOT(sceneCouldChange()));
	static_cast<QHBoxLayout *>(layout())->addWidget(drawWidget_, 1);

	for (std::map<int, bool>::const_iterator i = showOptions_.begin(); i != showOptions_.end(); i++) {
		drawing_->setOption(static_cast<Drawing::Option>(i->first), i->second);
	}
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

	drawing_ = new Drawing(statusText_, helpText_);

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

		QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Discard, Qt::Horizontal, errorDialog);

		layout->addWidget(textEdit);
		layout->addWidget(buttons);

		errorDialog->exec();
		delete errorDialog;

		delete drawing_;
		drawing_ = 0;
	} else {
		drawWidget_ = new DrawWidget(drawing_, this);
		amountField_->setText(QString::number(drawing_->countWaypoints()));
		connect(drawWidget_, SIGNAL(mouseClicked()), this, SLOT(sceneCouldChange()));
		static_cast<QHBoxLayout *>(layout())->addWidget(drawWidget_, 1);

		for (std::map<int, bool>::const_iterator i = showOptions_.begin(); i != showOptions_.end(); i++) {
			drawing_->setOption(static_cast<Drawing::Option>(i->first), i->second);
		}
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
	} else if (sender == boxShowPath_) {
		option = Drawing::ShowPath;
	} else {
		option = Drawing::ShowNeighbours;
	}

	showOptions_[option] = state == Qt::Checked;
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
