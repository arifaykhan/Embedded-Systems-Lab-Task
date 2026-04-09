import sys
import serial
import time
import csv
import os
import random
from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                             QHBoxLayout, QLabel, QLineEdit, QPushButton, QTextEdit, QMessageBox)
from PyQt6.QtCore import QThread, pyqtSignal    
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure

# --- Serial Worker Thread ---
# We use a thread so the GUI doesn't "freeze" while waiting for the Arduino
class SerialWorker(QThread):
    result_received = pyqtSignal(str)

    def __init__(self, serial_port):
        super().__init__()
        self.serial_port = serial_port

    def run(self):
        while True:
            if self.serial_port and self.serial_port.in_waiting > 0:
                line = self.serial_port.readline().decode('utf-8').strip()
                if line.startswith("RESULT"):
                    self.result_received.emit(line)

# --- Main GUI Window ---
class ReactionGame(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Arduino Tug-of-War: Pro Edition")
        self.setMinimumSize(800, 600)

        # Game State
        self.p1_wins = 0
        self.p2_wins = 0
        
        # Initialize Serial
        try:
            self.ser = serial.Serial('COM6', 9600, timeout=1) # Adjust COM port
            self.worker = SerialWorker(self.ser)
            self.worker.result_received.connect(self.handle_arduino_result)
            self.worker.start()
        except Exception as e:
            print(f"Serial Error: {e}")
            self.ser = None

        self.init_ui()

    def init_ui(self):
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)

        # 1. Player Input Area
        input_layout = QHBoxLayout()
        self.p1_input = QLineEdit("Player 1")
        self.p2_input = QLineEdit("Player 2")
        input_layout.addWidget(QLabel("P1 Name:"))
        input_layout.addWidget(self.p1_input)
        input_layout.addWidget(QLabel("P2 Name:"))
        input_layout.addWidget(self.p2_input)
        main_layout.addLayout(input_layout)

        # 2. Controls & Score
        self.start_btn = QPushButton("START ROUND")
        self.start_btn.clicked.connect(self.send_start_command)
        main_layout.addWidget(self.start_btn)

        self.score_label = QLabel("Score: 0 - 0")
        self.score_label.setStyleSheet("font-size: 24px; font-weight: bold; color: #2c3e50;")
        main_layout.addWidget(self.score_label)

        # 3. Data Visualization (Matplotlib Canvas)
        self.figure = Figure(figsize=(5, 3), dpi=100)
        self.canvas = FigureCanvas(self.figure)
        main_layout.addWidget(self.canvas)

        # 4. Event Log
        self.log = QTextEdit()
        self.log.setReadOnly(True)
        main_layout.addWidget(self.log)

    def send_start_command(self):
        if self.ser:
            wait_time = random.randint(1, 20)
            self.ser.write(f"START:{wait_time}\n".encode())
            self.log.append(f"System: Countdown initiated ({wait_time}s)...")

    def handle_arduino_result(self, data):
        # Data format from Arduino: RESULT|W:1|T:245|F:0
        parts = data.split('|')
        winner_id = int(parts[1].split(':')[1])
        r_time = int(parts[2].split(':')[1])
        is_false = int(parts[3].split(':')[1])

        p1 = self.p1_input.text()
        p2 = self.p2_input.text()
        
        # Update Internal Score
        if winner_id == 1: self.p1_wins += 1
        else: self.p2_wins += 1
        
        self.score_label.setText(f"Score: {self.p1_wins} - {self.p2_wins}")
        self.log_to_file(p1, p2, winner_id, r_time, is_false)
        self.update_chart(p1)

        if self.p1_wins >= 3:
         QMessageBox.information(self, "Match Over", f"{p1} Wins the Match!")
         self.p1_wins, self.p2_wins = 0, 0 # Reset scores

    def log_to_file(self, p1, p2, winner_id, r_time, is_false):
        names = [p1, p2]
        for name in names:
            filename = f"{name}_data.csv"
            file_exists = os.path.isfile(filename)
            with open(filename, 'a', newline='') as f:
                writer = csv.writer(f)
                if not file_exists:
                    writer.writerow(["Timestamp", "Opponent", "ReactionMS", "Status"])
                
                status = "WIN" if (name == p1 and winner_id == 1) or (name == p2 and winner_id == 2) else "LOSS"
                if is_false and status == "LOSS": status = "FALSE START"
                
                opponent = p2 if name == p1 else p1
                writer.writerow([time.strftime("%H:%M:%S"), opponent, r_time, status])

    def update_chart(self, player_name):
        filename = f"{player_name}_data.csv"
        if not os.path.isfile(filename): return

        times = []
        with open(filename, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                if int(row["ReactionMS"]) > 0:
                    times.append(int(row["ReactionMS"]))

        self.figure.clear()
        ax = self.figure.add_subplot(111)
        ax.plot(times, 'r-o', label="Reaction Speed")
        ax.set_title(f"Performance History: {player_name}")
        ax.set_ylabel("Milliseconds")
        ax.legend()
        self.canvas.draw()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = ReactionGame()
    window.show()
    sys.exit(app.exec())
