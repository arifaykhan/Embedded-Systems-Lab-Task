import sys
import serial
import time
import pyqtgraph as pg
from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                             QHBoxLayout, QLabel, QListWidget)
from PyQt6.QtCore import QTimer, Qt
from datetime import datetime

class SoundAnalyzerGUI(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("LAB TASK 5: Sound Level Analyzer")
        self.resize(1000, 600)
        
        self.port = "COM6"  # Change to your port
        self.baud = 9600
        self.threshold = 59.4
        self.start_time = time.time()
        
        self.plot_data = [] # For the scrolling graph
        self.event_log = [] # Stores (Timestamp, Value) for threshold breaks
        
        self.init_ui()
        
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=0.1)
        except Exception as e:
            print(f"Serial Error: {e}")
            self.ser = None
          
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_all)
        self.timer.start(20)

    def init_ui(self):
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QHBoxLayout(central_widget)
      
        left_container = QVBoxLayout()
        self.status_label = QLabel("STATUS: NORMAL")
        self.status_label.setStyleSheet("font-size: 20px; font-weight: bold; color: #2ecc71;")
        
        self.plot_widget = pg.PlotWidget(title="Live Sound Waveform")
        self.plot_widget.setBackground('k')
        self.plot_widget.setYRange(0, 1024)
        self.curve = self.plot_widget.plot(pen=pg.mkPen(color='#00ff00', width=2))

        self.plot_widget = pg.PlotWidget(title="Live Sound Waveform")
        self.plot_widget.setBackground('k')
        self.plot_widget.setYRange(30, 85)

        self.thresh_line = pg.InfiniteLine(pos=self.threshold, angle=0, pen=pg.mkPen('r', width=2, style=Qt.PenStyle.DashLine))
        self.plot_widget.addItem(self.thresh_line)

        self.thresh_label = pg.TextItem(text=f"Threshold: {self.threshold}", color='r', anchor=(0, 1))
        self.thresh_label.setPos(0, self.threshold)
        self.plot_widget.addItem(self.thresh_label)

        self.curve = self.plot_widget.plot(pen=pg.mkPen(color='#00ff00', width=2))
        
        left_container.addWidget(self.status_label)
        left_container.addWidget(self.plot_widget)
        main_layout.addLayout(left_container, stretch=3)

        right_container = QVBoxLayout()
        right_container.addWidget(QLabel("THRESHOLD VIOLATIONS LOG:"))
        self.log_list = QListWidget()
        self.log_list.setStyleSheet("background: #222; color: #ff4444; font-family: monospace;")
        right_container.addWidget(self.log_list)
        main_layout.addLayout(right_container, stretch=1)

    def update_all(self):
        if self.ser and self.ser.in_waiting:
            try:
                line = self.ser.readline().decode('utf-8').strip()
                if line:
                    value = float(line)
                    self.process_data(value)
            except Exception:
                pass

    def process_data(self, val):
        self.plot_data.append(val)
        if len(self.plot_data) > 200: # Keep last 200 samples
            self.plot_data.pop(0)
        self.curve.setData(self.plot_data)
      
        if val > self.threshold:
            timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
            entry = f"[{timestamp}] LOUD: {val}"
            
            self.event_log.append((timestamp, val))
            
            self.log_list.addItem(entry)
            self.log_list.scrollToBottom()
            
            self.status_label.setText("STATUS: LOUD EVENT DETECTED!")
            self.status_label.setStyleSheet("font-size: 20px; font-weight: bold; color: #e74c3c;")
        else:
            self.status_label.setText("STATUS: NORMAL")
            self.status_label.setStyleSheet("font-size: 20px; font-weight: bold; color: #2ecc71;")

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = SoundAnalyzerGUI()
    window.show()
    sys.exit(app.exec())
