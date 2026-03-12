import sys
import serial
import time
import pyqtgraph as pg
from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QLabel, QGridLayout)
from PyQt6.QtCore import QTimer, Qt
from collections import deque

class JoystickUI(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("LAB TASK 4 - Joystick Analyzer")
        self.setStyleSheet("background-color: #00000;")
        
        # Serial config
        self.serial_port = None
        self.baud_rate = 9600
        self.port_name = "COM6"  # change this to your port
        
        # Data storage
        self.x_data = deque(maxlen=50) # "Ghost trail" length
        self.y_data = deque(maxlen=50)
        self.start_time = time.time()
        self.packet_count = 0
        self.current_rate = 0

        # UI construction
        self.init_ui()
        
        # Timer for UI updates
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_all)
        self.timer.start(10) # 100Hz UI refresh

    def init_ui(self):
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        outer_layout = QVBoxLayout(main_widget)

        # 1. Header and toggle
        header_layout = QHBoxLayout()
        self.title_label = QLabel("LAB TASK 4")
        self.title_label.setStyleSheet("font-size: 24px; font-weight: bold; color: #1a5fb4;")
        
        self.toggle_btn = QPushButton("START TEST")
        self.toggle_btn.setCheckable(True)
        self.toggle_btn.clicked.connect(self.toggle_serial)
        self.toggle_btn.setStyleSheet("padding: 10px; font-weight: bold;")
        
        header_layout.addWidget(self.title_label)
        header_layout.addStretch()
        header_layout.addWidget(self.toggle_btn)
        outer_layout.addLayout(header_layout)

        # 2. Main visualization area
        content_layout = QHBoxLayout()
        
        # LEFT: 2D plot (visualization)
        self.plot_widget = pg.PlotWidget(title="Joystick Path (XY Plane)")
        self.plot_widget.setBackground('w')
        self.plot_widget.setXRange(0, 1023)
        self.plot_widget.setYRange(0, 1023)
        self.trail_line = self.plot_widget.plot(pen=pg.mkPen(color='#3498db', width=2, style=Qt.PenStyle.DotLine))
        self.current_pos = self.plot_widget.plot(symbol='o', symbolSize=12, symbolBrush='#e74c3c')
        content_layout.addWidget(self.plot_widget, stretch=2)

        # RIGHT: Button mapping (cross)
        self.grid_layout = QGridLayout()
        self.btns = {
            "UP": QLabel("UP"), "DOWN": QLabel("DOWN"),
            "LEFT": QLabel("LEFT"), "RIGHT": QLabel("RIGHT"), "CTR": QLabel("BTN")
        }
        for key, lbl in self.btns.items():
            lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
            lbl.setStyleSheet("border: 2px solid gray; background: #ddd; min-width: 60px; min-height: 60px;")
        
        self.grid_layout.addWidget(self.btns["UP"], 0, 1)
        self.grid_layout.addWidget(self.btns["LEFT"], 1, 0)
        self.grid_layout.addWidget(self.btns["CTR"], 1, 1)
        self.grid_layout.addWidget(self.btns["RIGHT"], 1, 2)
        self.grid_layout.addWidget(self.btns["DOWN"], 2, 1)
        content_layout.addLayout(self.grid_layout, stretch=1)
        
        outer_layout.addLayout(content_layout)

        # 3. Footer data
        footer_layout = QHBoxLayout()
        self.volt_label = QLabel("X: 0.00V | Y: 0.00V")
        self.rate_label = QLabel("Sample Rate: 0 Hz")
        footer_layout.addWidget(self.volt_label)
        footer_layout.addStretch()
        footer_layout.addWidget(self.rate_label)
        outer_layout.addLayout(footer_layout)

    def toggle_serial(self, checked):
        if checked:
            try:
                self.serial_port = serial.Serial(self.port_name, self.baud_rate, timeout=0.1)
                self.toggle_btn.setText("STOP TEST")
                self.toggle_btn.setStyleSheet("background-color: #2ecc71; color: white;")
            except Exception as e:
                print(f"Error: {e}")
                self.toggle_btn.setChecked(False)
        else:
            if self.serial_port: self.serial_port.close()
            self.toggle_btn.setText("START TEST")
            self.toggle_btn.setStyleSheet("")

    def update_all(self):
        if self.serial_port and self.serial_port.is_open:
            while self.serial_port.in_waiting:
                line = self.serial_port.readline().decode('utf-8').strip()
                try:
                    vals = line.split(',')
                    if len(vals) == 3:
                        x, y, btn = int(vals[0]), int(vals[1]), int(vals[2])
                        
                        # Update plot
                        self.x_data.append(x)
                        self.y_data.append(y)
                        self.trail_line.setData(list(self.x_data), list(self.y_data))
                        self.current_pos.setData([x], [y])
                        
                        # Update voltages
                        vx, vy = (x * 5.0 / 1023), (y * 5.0 / 1023)
                        self.volt_label.setText(f"X: {vx:.2f}V | Y: {vy:.2f}V")
                        
                        # Update button mapping colors
                        self.update_mapping(x, y, btn)
                        
                        # Track sample rate
                        self.packet_count += 1
                except: pass

        # Update sample rate every second
        elapsed = time.time() - self.start_time
        if elapsed > 1.0:
            self.rate_label.setText(f"Sample Rate: {self.packet_count} Hz")
            self.packet_count = 0
            self.start_time = time.time()

    def update_mapping(self, x, y, btn):
        for lbl in self.btns.values(): lbl.setStyleSheet("border: 2px solid gray; background: #ddd;")
        
        if y > 600: self.btns["UP"].setStyleSheet("background: #2ecc71; font-weight: bold;")
        elif y < 400: self.btns["DOWN"].setStyleSheet("background: #2ecc71; font-weight: bold;")
        
        if x < 400: self.btns["LEFT"].setStyleSheet("background: #2ecc71; font-weight: bold;")
        elif x > 600: self.btns["RIGHT"].setStyleSheet("background: #2ecc71; font-weight: bold;")
        
        if btn: self.btns["CTR"].setStyleSheet("background: #f1c40f; font-weight: bold;")

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = JoystickUI()
    window.show()
    sys.exit(app.exec())
