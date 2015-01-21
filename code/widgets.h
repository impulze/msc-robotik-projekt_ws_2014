#ifndef ROB_WIDGETS_H_INCLUDED
#define ROB_WIDGETS_H_INCLUDED

#include <QtWidgets/QMainWindow>

class Drawing;
class DrawWidget;
class QCheckBox;
class QComboBox;
class QLineEdit;
class QPushButton;
class QTextEdit;
class QXmlStreamReader;
class QXmlStreamWriter;
class Stats;

class MainWindow
	: public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = 0);

private:
	MainWindow(MainWindow const &other);
	MainWindow &operator=(MainWindow const &other);
};

class CentralWidget
	: public QWidget
{
	Q_OBJECT

public:
	CentralWidget(QWidget *parent = 0);
	~CentralWidget();

	bool loadProject(QXmlStreamReader *reader);
	bool saveProject(QXmlStreamWriter *writer);

private:
	CentralWidget(CentralWidget const &other);
	CentralWidget &operator=(CentralWidget const &other);

private Q_SLOTS:
	void wantsRoomLoaded();
	void wantsProjectLoaded();
	void wantsProjectSaved();
	void checkBoxChanged(int state);
	void buttonClicked();
	void amountOfNodesChanged();
	void sceneCouldChange();

private:
	void removeRoom();
	void createDrawWidget();
	void createNewDrawing();
	bool checkBoxEvent(QObject *object, QEvent *event);

private:
	Drawing *drawing_;
	DrawWidget *drawWidget_;
	QLineEdit *amountField_;
	QCheckBox *boxAdd_;
	QCheckBox *boxDel_;
	QCheckBox *boxStart_;
	QCheckBox *boxEnd_;
	QCheckBox *boxShowTri_;
	QCheckBox *boxShowRoomTri_;
	QCheckBox *boxShowWay_;
	QCheckBox *boxShowPath_;
	QCheckBox *boxShowNeighbours_;
	QComboBox *boxAlgorithms_;
	QPushButton *buttonAnimate_;
	QPushButton *buttonStats_;
	QTextEdit *statusText_;
	QTextEdit *helpText_;
	Stats *stats_;

	friend class CheckBoxEventFilter;
};

class CheckBoxEventFilter
	: public QObject
{
	Q_OBJECT

public:
	CheckBoxEventFilter(CentralWidget *central);

	bool eventFilter(QObject *object, QEvent *event);

private:
	CentralWidget *central_;
};

#endif // ROB_WIDGETS_H_INCLUDED
