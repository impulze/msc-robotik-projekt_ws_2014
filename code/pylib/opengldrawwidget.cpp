#include "opengldrawwidget.h"

#include <iostream>

OpenGLDrawWidget::OpenGLDrawWidget(Drawing *drawing, QWidget *parent)
	: QOpenGLWidget(parent)
{
	setAutoFillBackground(false);
}

OpenGLDrawWidget::~OpenGLDrawWidget()
{
}

void OpenGLDrawWidget::initializeGL()
{
	glClearColor(0.5f, 1.0f, 0.3f, 1.0f);
}

void OpenGLDrawWidget::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glColor3f(0.0f, 0.5f, 1.0f);
	glRectf(-0.75f, 0.75f, 0.75f, -0.75f);
}

void OpenGLDrawWidget::resizeGL(int width, int height)
{
	std::cout << "resizeGL\n";
}
