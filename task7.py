import serial
import sqlite3
import threading
import csv
import os
import tkinter as tk
from tkinter import ttk, messagebox
from datetime import datetime

# --- CONFIGURATION ---
PORT = 'COM4'  
BAUD = 9600
CSV_FILE = "security_logs.csv"

# --- DATABASE SETUP ---
def init_db():
    conn = sqlite3.connect("security_system.db")
    cursor = conn.cursor()
    cursor.execute('''CREATE TABLE IF NOT EXISTS tags 
                      (tag_id TEXT PRIMARY KEY, scan_count INTEGER, last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP)''')
    conn.commit()
    conn.close()

def log_to_csv(tag_id, count):
    """Saves the scan entry to a CSV file."""
    file_exists = os.path.isfile(CSV_FILE)
    
    with open(CSV_FILE, mode='a', newline='') as file:
        writer = csv.writer(file)
        # Write header if the file is new
        if not file_exists:
            writer.writerow(["Tag ID", "Scan Count", "Timestamp"])
        
        # Write the data row
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        writer.writerow([tag_id, count, timestamp])

def log_tag(tag_id):
    conn = sqlite3.connect("security_system.db")
    cursor = conn.cursor()
    
    cursor.execute("SELECT scan_count FROM tags WHERE tag_id = ?", (tag_id,))
    row = cursor.fetchone()
    
    if row:
        new_count = row[0] + 1
        cursor.execute("UPDATE tags SET scan_count = ?, last_seen = CURRENT_TIMESTAMP WHERE tag_id = ?", (new_count, tag_id))
    else:
        new_count = 1
        cursor.execute("INSERT INTO tags (tag_id, scan_count) VALUES (?, 1)", (tag_id,))
    
    conn.commit()
    conn.close()
    
    # Trigger CSV logging
    log_to_csv(tag_id, new_count)

# --- GUI CLASS ---
class SecurityGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("RFID Security Logger (DB + CSV)")
        self.root.geometry("600x450")

        # UI Elements
        self.label = tk.Label(root, text="Security System Database", font=("Arial", 16, "bold"))
        self.label.pack(pady=10)

        # Table (Treeview)
        self.tree = ttk.Treeview(root, columns=("ID", "Count", "Last Seen"), show='headings')
        self.tree.heading("ID", text="Tag ID")
        self.tree.heading("Count", text="Scan Count")
        self.tree.heading("Last Seen", text="Last Scanned At")
        self.tree.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        self.refresh_btn = tk.Button(root, text="Manual Refresh", command=self.update_table)
        self.refresh_btn.pack(pady=5)
        
        self.csv_label = tk.Label(root, text=f"Logging to: {CSV_FILE}", font=("Arial", 8), fg="gray")
        self.csv_label.pack()

        self.status_var = tk.StringVar(value="Status: Waiting for Data...")
        self.status_bar = tk.Label(root, textvariable=self.status_var, bd=1, relief=tk.SUNKEN, anchor=tk.W)
        self.status_bar.pack(side=tk.BOTTOM, fill=tk.X)

        init_db()
        self.update_table()
        
        self.thread = threading.Thread(target=self.listen_to_arduino, daemon=True)
        self.thread.start()

    def update_table(self):
        for item in self.tree.get_children():
            self.tree.delete(item)
        
        conn = sqlite3.connect("security_system.db")
        cursor = conn.cursor()
        cursor.execute("SELECT * FROM tags ORDER BY last_seen DESC")
        for row in cursor.fetchall():
            self.tree.insert("", tk.END, values=row)
        conn.close()

    def listen_to_arduino(self):
        try:
            ser = serial.Serial(PORT, BAUD, timeout=1)
            while True:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line.startswith("TAG:"):
                        tag_id = line.split(":")[1]
                        log_tag(tag_id)
                        self.status_var.set(f"Last Scanned: {tag_id} (Logged to CSV)")
                        self.root.after(0, self.update_table)
        except Exception as e:
            self.status_var.set(f"Error: {e}")
            # Use after() to show messagebox from a background thread safely
            self.root.after(0, lambda: messagebox.showerror("Serial Error", f"Could not connect to {PORT}"))

if __name__ == "__main__":
    root = tk.Tk()
    app = SecurityGUI(root)
    root.mainloop()
