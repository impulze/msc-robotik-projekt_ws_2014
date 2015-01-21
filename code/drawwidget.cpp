#include "drawing.h"
#include "drawwidget.h"
#include "opengldrawwidget.h"

#include <QtCore/QEvent>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QHBoxLayout>

DrawWidget::DrawWidget(Drawing *drawing, QWidget *parent)
	: QWidget(parent),
	  drawing_(drawing)
{
	new QHBoxLayout(this);

	layout()->setContentsMargins(0, 0, 0, 0);
	layout()->addWidget(new OpenGLDrawWidget(drawing, this));

	installEventFilter(this);
}

DrawWidget::~DrawWidget()
{
}

bool DrawWidget::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::MouseButtonRelease) {
		QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

		if (mouseEvent->button() == Qt::LeftButton) {
			drawing_->mouseClick(mouseEvent->x(), mouseEvent->y());
			emit mouseClicked();
		}
	}

	return QObject::eventFilter(object, event);
}
