import serial
import sqlite3
import threading
import tkinter as tk
from tkinter import ttk, messagebox

# --- CONFIGURATION ---
PORT = 'COM4'  # Change this to your Arduino's port (e.g., /dev/ttyUSB0 on Linux)
BAUD = 9600

# --- DATABASE SETUP ---
def init_db():
    conn = sqlite3.connect("security_system.db")
    cursor = conn.cursor()
    cursor.execute('''CREATE TABLE IF NOT EXISTS tags 
                      (tag_id TEXT PRIMARY KEY, scan_count INTEGER, last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP)''')
    conn.commit()
    conn.close()

def log_tag(tag_id):
    conn = sqlite3.connect("security_system.db")
    cursor = conn.cursor()
    # Check if tag exists
    cursor.execute("SELECT scan_count FROM tags WHERE tag_id = ?", (tag_id,))
    row = cursor.fetchone()
    
    if row:
        # Increment count
        new_count = row[0] + 1
        cursor.execute("UPDATE tags SET scan_count = ?, last_seen = CURRENT_TIMESTAMP WHERE tag_id = ?", (new_count, tag_id))
    else:
        # New entry
        cursor.execute("INSERT INTO tags (tag_id, scan_count) VALUES (?, 1)", (tag_id,))
    
    conn.commit()
    conn.close()

# --- GUI CLASS ---
class SecurityGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("RFID Security Logger")
        self.root.geometry("600x400")

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

        self.status_var = tk.StringVar(value="Status: Waiting for Data...")
        self.status_bar = tk.Label(root, textvariable=self.status_var, bd=1, relief=tk.SUNKEN, anchor=tk.W)
        self.status_bar.pack(side=tk.BOTTOM, fill=tk.X)

        init_db()
        self.update_table()
        
        # Start Serial Thread
        self.thread = threading.Thread(target=self.listen_to_arduino, daemon=True)
        self.thread.start()

    def update_table(self):
        # Clear current rows
        for item in self.tree.get_children():
            self.tree.delete(item)
        
        # Pull from DB
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
                    line = ser.readline().decode('utf-8').strip()
                    if line.startswith("TAG:"):
                        tag_id = line.split(":")[1]
                        log_tag(tag_id)
                        self.status_var.set(f"Last Scanned: {tag_id}")
                        # Schedule table update in the main thread
                        self.root.after(0, self.update_table)
        except Exception as e:
            self.status_var.set(f"Error: {e}")
            messagebox.showerror("Serial Error", f"Could not connect to {PORT}. Check your connection.")

if __name__ == "__main__":
    root = tk.Tk()
    app = SecurityGUI(root)
    root.mainloop()
