%Import QtWidgets/QtWidgetsmod.sip

class DrawWidget
	: public QWidget
{
%TypeHeaderCode
#include <drawwidget.h>
%End

public:
	DrawWidget(Drawing *drawing, QWidget *parent = 0);
	~DrawWidget();
};
