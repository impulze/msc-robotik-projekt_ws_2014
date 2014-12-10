#include "drawing.h"
#include "opengldrawwidget.h"

#include <QTimer>
#include <iostream>

OpenGLDrawWidget::OpenGLDrawWidget(Drawing *drawing, QWidget *parent)
	: QOpenGLWidget(parent),
	  drawing_(drawing)
{
	setAutoFillBackground(false);

	QTimer *timer = new QTimer;
	connect(timer, SIGNAL(timeout()), this, SLOT(update()));
	timer->start(0);
}

OpenGLDrawWidget::~OpenGLDrawWidget()
{
}

void OpenGLDrawWidget::initializeGL()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	drawing_->initialize();
}

void OpenGLDrawWidget::paintGL()
{
	drawing_->paint();
}

void OpenGLDrawWidget::resizeGL(int width, int height)
{
	drawing_->resize(width, height);
}
