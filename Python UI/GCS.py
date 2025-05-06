# Ground Control Station Simulator

import tkinter as tk
from tkinter.scrolledtext import ScrolledText
import serial
import threading

# Configure serial port
SERIAL_PORT = 'COM6'
BAUD_RATE = 115200

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
except serial.SerialException as e:
    print(f"Error opening serial port: {e}")
    exit(1)

# GUI setup
root = tk.Tk()
root.title("STM32 Telemetry Monitor")

# Telemetry display
output_box = ScrolledText(root, wrap=tk.WORD, width=80, height=20)
output_box.pack(padx=10, pady=(10, 0))

# Command entry
cmd_frame = tk.Frame(root)
cmd_frame.pack(padx=10, pady=(5, 10), fill=tk.X)

cmd_entry = tk.Entry(cmd_frame, width=70)
cmd_entry.pack(side=tk.LEFT, fill=tk.X, expand=True)

def send_command():
    cmd = cmd_entry.get()
    if cmd and ser.is_open:
        ser.write((cmd + '\r\n').encode('utf-8'))
        output_box.insert(tk.END, f"> {cmd}\n")
        output_box.see(tk.END)
        cmd_entry.delete(0, tk.END)

send_btn = tk.Button(cmd_frame, text="Send", command=send_command)
send_btn.pack(side=tk.RIGHT)

def on_enter(event):
    send_command()

cmd_entry.bind("<Return>", on_enter)

# Background serial reading
def read_serial():
    while True:
        if ser.in_waiting:
            try:
                line = ser.readline().decode('utf-8', errors='replace')
                output_box.insert(tk.END, line)
                output_box.see(tk.END)
            except Exception as e:
                output_box.insert(tk.END, f"Error: {e}\n")

thread = threading.Thread(target=read_serial, daemon=True)
thread.start()

# Start GUI
root.mainloop()
