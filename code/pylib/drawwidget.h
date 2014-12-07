#ifndef DRAWWIDGET_H_INCLUDED
#define DRAWWIDGET_H_INCLUDED

#include <QWidget>

class DrawWidget
	: public QWidget
{
	Q_OBJECT

public:
	DrawWidget(QWidget *parent = 0);
	~DrawWidget();

private:
	DrawWidget(const DrawWidget &other);
	DrawWidget &operator=(const DrawWidget &other);
};

#endif // DRAWWIDGET_H_INCLUDED
