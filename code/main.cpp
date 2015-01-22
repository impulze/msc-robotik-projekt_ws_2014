#include "widgets.h"

#include <QtCore/QTimer>
#include <QtWidgets/QApplication>

#include <csignal>
#include <cstdio>
#include <stdexcept>

namespace
{
	QApplication *gApplication;

	void signalHandler(int signal);
	void dummy();
}

int main(int argc, char **argv)
{
	QApplication application(argc, argv);
	MainWindow mainWindow;

	mainWindow.show();

	void (*sigResult)(int) = std::signal(SIGINT, signalHandler);

	if (sigResult == SIG_ERR) {
		throw std::runtime_error("Cannot install signal handler.");
	}

	// this is needed because otherwise we can't catch the signal above
	QTimer timer;
	//QObject::connect(&timer, SIGNAL(timeout()), &dummy, SLOT(dummy()));
	timer.start(1000);

	int result = application.exec();
	return result;
}

namespace
{
	void signalHandler(int signal)
	{
		std::fprintf(stderr, "Exiting due to signal SIGINT.\n");
		gApplication->quit();
	}
}
