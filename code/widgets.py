from PyQt5 import QtWidgets, QtGui, QtCore

import native

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

		self.setCentralWidget(CentralWidget(self))

	def wantsRoomOpen(self):
		filename = QtWidgets.QFileDialog.getOpenFileName(self, self.tr('Open room image'), '', 'Images (*.png)')

		if len(filename[0]) == 0:
			return


		print(filename)

	def wantsRoomSave(self):
		filename = QtWidgets.QFileDialog.getSaveFileName(self, self.tr('Save room image'), '', 'Images (*.png)')

		print(filename)

class CentralWidget(QtWidgets.QWidget):
	drawWidget = None

	def __init__(self, parent = None):
		super(CentralWidget, self).__init__(parent)

		self.resize(800, 600)

		amountLabel = QtWidgets.QLabel(self.tr('Amount of nodes'), self)
		amountField = QtWidgets.QLineEdit(self)
		amountField.setValidator(QtGui.QIntValidator(2, 20000, self))
		amountField.editingFinished.connect(self.amountOfNodesChanged)

		amountLayout = QtWidgets.QHBoxLayout()
		amountLayout.addWidget(amountLabel)
		amountLayout.addWidget(amountField)

		self.drawing = native.Drawing()
		self.drawWidget = native.DrawWidget(self.drawing, self)

		sideLayout = QtWidgets.QVBoxLayout()
		sideLayout.addLayout(amountLayout)
		sideLayout.addItem(QtWidgets.QSpacerItem(1, 1, QtWidgets.QSizePolicy.MinimumExpanding, QtWidgets.QSizePolicy.MinimumExpanding))

		mainLayout = QtWidgets.QHBoxLayout(self)
		mainLayout.addLayout(sideLayout)
		mainLayout.addWidget(self.drawWidget, 1)

	def amountOfNodesChanged(self):
		"""
		try:
			num = int(lineEdit.text())
		except ValueError as error:
			msg = self.tr('Invalid amount of nodes entered.\n')
			msg += self.tr('Only numbers are accepted.\n')
			msg += str(error)
			QtWidgets.QMessageBox.critical(self, self.tr('Invalid input'), msg)
			lineEdit.setFocus()
			return
		"""

		lineEdit = self.sender()
		num = int(lineEdit.text())

		print('The amount of nodes changed \'%d\'' % num)

