#ifndef ROB_OPENGLDRAWWIDGET_H_INCLUDED
#define ROB_OPENGLDRAWWIDGET_H_INCLUDED

#include <QtWidgets/QOpenGLWidget>

class Drawing;

class OpenGLDrawWidget
	: public QOpenGLWidget
{
	Q_OBJECT

public:
	OpenGLDrawWidget(Drawing *drawing, QWidget *parent = 0);
	~OpenGLDrawWidget();

private:
	OpenGLDrawWidget(const OpenGLDrawWidget &other);
	OpenGLDrawWidget &operator=(const OpenGLDrawWidget &other);

private:
	void initializeGL();
	void paintGL();
	void resizeGL(int width, int height);

private:
	Drawing *drawing_;
};

#endif // ROB_OPENGLDRAWWIDGET_H_INCLUDED
