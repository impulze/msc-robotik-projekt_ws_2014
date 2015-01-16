#ifndef ROB_DRAWWIDGET_H_INCLUDED
#define ROB_DRAWWIDGET_H_INCLUDED

#include <QtWidgets/QWidget>

class Drawing;

class DrawWidget
	: public QWidget
{
	Q_OBJECT

public:
	DrawWidget(Drawing *drawing, QWidget *parent = 0);
	~DrawWidget();

private:
	DrawWidget(const DrawWidget &other);
	DrawWidget &operator=(const DrawWidget &other);

	bool eventFilter(QObject *object, QEvent *event);

	Drawing *drawing_;
};

#endif // ROB_DRAWWIDGET_H_INCLUDED
