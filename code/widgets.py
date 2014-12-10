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
		self.layout().setContentsMargins(0, 0, 0, 0)
		self.setCentralWidget(CentralWidget(self))

		self.resize(800, 600)

	def wantsRoomOpen(self):
		filename = QtWidgets.QFileDialog.getOpenFileName(self, self.tr('Open room image'), '', 'Images (*.png)')

		if len(filename[0]) == 0:
			return


		print(filename)

	def wantsRoomSave(self):
		filename = QtWidgets.QFileDialog.getSaveFileName(self, self.tr('Save room image'), '', 'Images (*.png)')

		print(filename)

class DrawWidget(native.DrawWidget):
	drawing = None

	def __init__(self, drawing, parent):
		super(DrawWidget, self).__init__(drawing, parent)

		self.drawing = drawing
		self.installEventFilter(self)

	def eventFilter(self, source, event):
		if event.type() == QtCore.QEvent.MouseButtonRelease:
			self.drawing.mouseClick(event.x(), event.y())

		return native.DrawWidget.eventFilter(self, source, event)

class CentralWidget(QtWidgets.QWidget):
	drawing = native.Drawing()
	drawWidget = None
	amountButtonAdd = None
	amountButtonDel = None

	def __init__(self, parent):
		super(CentralWidget, self).__init__(parent)

		self.drawWidget = DrawWidget(self.drawing, self)

		amountLabel = QtWidgets.QLabel(self.tr('Amount of nodes'), self)
		amountField = QtWidgets.QLineEdit(self)
		amountField.setValidator(QtGui.QIntValidator(2, 20000, self))
		amountField.returnPressed.connect(self.amountOfNodesChanged)
		amountSlider = QtWidgets.QSlider(QtCore.Qt.Horizontal, self)
		self.amountButtonAdd = QtWidgets.QCheckBox(self.tr('Add waypoint'), self)
		self.amountButtonDel = QtWidgets.QCheckBox(self.tr('Remove waypoint'), self)
		self.amountButtonAdd.stateChanged.connect(self.addWaypointChanged)
		self.amountButtonDel.stateChanged.connect(self.delWaypointChanged)

		amountLayout = QtWidgets.QVBoxLayout()
		amountTopLayout = QtWidgets.QHBoxLayout()
		amountTopLayout.addWidget(amountLabel)
		amountTopLayout.addWidget(amountField)
		amountLayout.addLayout(amountTopLayout)
		amountLayout.addWidget(amountSlider)
		amountLayout.addWidget(self.amountButtonAdd)
		amountLayout.addWidget(self.amountButtonDel)

		sideLayout = QtWidgets.QVBoxLayout()
		sideLayout.addLayout(amountLayout)
		sideLayout.addItem(QtWidgets.QSpacerItem(1, 1, QtWidgets.QSizePolicy.MinimumExpanding, QtWidgets.QSizePolicy.MinimumExpanding))

		mainLayout = QtWidgets.QHBoxLayout(self)
		mainLayout.addLayout(sideLayout)
		mainLayout.addWidget(self.drawWidget, 1)

	def addWaypointChanged(self, state):
		if state == QtCore.Qt.Checked:
			if self.amountButtonDel.checkState() == QtCore.Qt.Checked:
				self.amountButtonDel.setCheckState(QtCore.Qt.Unchecked)

			self.drawing.setWaypointModification(native.Drawing.WaypointAdd)

	def delWaypointChanged(self, state):
		if state == QtCore.Qt.Checked:
			if self.amountButtonAdd.checkState() == QtCore.Qt.Checked:
				self.amountButtonAdd.setCheckState(QtCore.Qt.Unchecked)

			self.drawing.setWaypointModification(native.Drawing.WaypointDelete)

	def amountOfNodesChanged(self):
		lineEdit = self.sender()
		num = int(lineEdit.text())

		print('The amount of nodes changed \'%d\'' % num)

		self.drawing.setNodes(num)
