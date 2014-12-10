#include "drawwidget.h"
#include "opengldrawwidget.h"

#include <QHBoxLayout>

DrawWidget::DrawWidget(Drawing *drawing, QWidget *parent)
	: QWidget(parent)
{
	new QHBoxLayout(this);

	layout()->setContentsMargins(0, 0, 0, 0);
	layout()->addWidget(new OpenGLDrawWidget(drawing, this));
}

DrawWidget::~DrawWidget()
{
}
