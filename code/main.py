#!/usr/bin/env python3

import os, signal, sys

sys.path.append(os.path.dirname(os.path.realpath(__file__)) + '/pylib')

from PyQt5 import QtCore, QtWidgets

import widgets

app = None

def signal_handler(signal, frame):
	global app
	print('Exiting due to signal SIGINT.')
	app.quit()

def main():
	global app
	signal.signal(signal.SIGINT, signal_handler)
	app = QtWidgets.QApplication(sys.argv)
	mw = widgets.MainWindow()

	mw.show()

	# This is needed because otherwise we can't catch the signal above
	timer = QtCore.QTimer()
	timer.timeout.connect(lambda: None)
	timer.start(1000)

	result = app.exec_()
	sys.exit(result)

if __name__ == '__main__':
	main()
