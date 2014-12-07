#include "drawwidget.h"
#include "opengldrawwidget.h"

#include <QHBoxLayout>

DrawWidget::DrawWidget(Drawing *drawing, QWidget *parent)
	: QWidget(parent)
{
	new QHBoxLayout(this);

	layout()->addWidget(new OpenGLDrawWidget(drawing, this));
}

DrawWidget::~DrawWidget()
{
}
