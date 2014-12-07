from PyQt5 import QtWidgets, QtGui, QtCore

import nativewidgets

class MainWindow(QtWidgets.QMainWindow):
	def __init__(self, parent = None):
		super(MainWindow, self).__init__(parent)

		menuRoom = self.menuBar().addMenu(self.tr('&Room'))
		actRoomNew = menuRoom.addAction(self.tr('&New'))
		actRoomLoad = menuRoom.addAction(self.tr('&Load'))
		actRoomLoad.triggered.connect(self.wantsRoomOpen)
		actRoomSave = menuRoom.addAction(self.tr('&Save'))
		actRoomSave.triggered.connect(self.wantsRoomSave)


		menuRobot = self.menuBar().addMenu(self.tr('R&obot'))
		actRobotSimulate = menuRobot.addAction(self.tr('&Simulate'))
		#actRobotAnalyze = menuRobot.addAction(self.tr('&Analyze'))
		actRobotExport = menuRobot.addAction(self.tr('&Export Simulation'))

		actQuit = self.menuBar().addAction(self.tr('&Quit'))
		actQuit.triggered.connect(self.close)

		self.setWindowTitle('Simulation')

		self.setCentralWidget(nativewidgets.DrawWidget(self))
		#self.setCentralWidget(CentralWidget(self))

	def wantsRoomOpen(self):
		filename = QtWidgets.QFileDialog.getOpenFileName(self, self.tr('Open room image'), '', 'Images (*.png)')

		if len(filename[0]) == 0:
			return


		print(filename)

	def wantsRoomSave(self):
		filename = QtWidgets.QFileDialog.getSaveFileName(self, self.tr('Save room image'), '', 'Images (*.png)')

		print(filename)

class CentralWidget(QtWidgets.QWidget):
	def __init__(self, parent = None):
		super(CentralWidget, self).__init__(parent)

		drawWidget = nativewidgets.DrawWidget(self)

		layout = QtWidgets.QVBoxLayout(self)
		layout.addWidget(drawWidget)
