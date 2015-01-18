#ifndef ROB_WIDGETS_H_INCLUDED
#define ROB_WIDGETS_H_INCLUDED

#include <QtWidgets/QMainWindow>

class Drawing;
class DrawWidget;
class QCheckBox;
class QTextEdit;

class MainWindow
	: public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = 0);

private:
	MainWindow(MainWindow const &other);
	MainWindow &operator=(MainWindow const &other);

private Q_SLOTS:
	void wantsRoomLoaded();
	void wantsRoomSaved();
};

class CentralWidget
	: public QWidget
{
	Q_OBJECT

public:
	CentralWidget(QWidget *parent = 0);

private:
	CentralWidget(CentralWidget const &other);
	CentralWidget &operator=(CentralWidget const &other);

private Q_SLOTS:
	void checkBoxChanged(int state);
	void amountOfNodesChanged();

private:
	bool checkBoxEvent(QObject *object, QEvent *event);

private:
	Drawing *drawing_;
	DrawWidget *drawWidget_;
	QCheckBox *boxAdd_;
	QCheckBox *boxDel_;
	QCheckBox *boxStart_;
	QCheckBox *boxEnd_;
	QCheckBox *boxShowTri_;
	QCheckBox *boxShowRoomTri_;
	QCheckBox *boxShowWay_;
	QCheckBox *boxShowPath_;
	QTextEdit *infoTextEdit_;

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
