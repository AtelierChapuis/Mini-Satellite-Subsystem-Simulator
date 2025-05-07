import tkinter as tk
from tkinter import ttk, scrolledtext, simpledialog
import serial
import threading
import time

class STM32CommunicationApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Satellite Ground Control Station Simulator")
        self.root.geometry("800x600")
        
        self.serial_port = None
        self.is_connected = False
        self.receiving_thread = None
        self.stop_thread = False
        
        # Added delay between commands (in seconds)
        self.tx_delay = 0.1
        
        self.create_widgets()
        self.port_refresh()
        
    def create_widgets(self):
        # Frame for connection settings
        conn_frame = ttk.LabelFrame(self.root, text="Connection Settings")
        conn_frame.pack(fill=tk.X, padx=10, pady=5)
        
        # Port selection
        ttk.Label(conn_frame, text="Port:").grid(row=0, column=0, sticky=tk.W, padx=5, pady=5)
        self.port_combobox = ttk.Combobox(conn_frame, width=20)
        self.port_combobox.grid(row=0, column=1, padx=5, pady=5)
        
        # Refresh button
        ttk.Button(conn_frame, text="Refresh", command=self.port_refresh).grid(row=0, column=2, padx=5, pady=5)
        
        # Baud rate selection
        ttk.Label(conn_frame, text="Baud Rate:").grid(row=0, column=3, sticky=tk.W, padx=5, pady=5)
        self.baud_combobox = ttk.Combobox(conn_frame, width=10, 
                                          values=["9600", "19200", "38400", "57600", "115200"])
        self.baud_combobox.current(4)  # Default to 115200
        self.baud_combobox.grid(row=0, column=4, padx=5, pady=5)
        
        # Connect button
        self.connect_button = ttk.Button(conn_frame, text="Connect", command=self.toggle_connection)
        self.connect_button.grid(row=0, column=5, padx=5, pady=5)
        
        # Terminal output
        terminal_frame = ttk.LabelFrame(self.root, text="Terminal")
        terminal_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        
        self.terminal = scrolledtext.ScrolledText(terminal_frame, wrap=tk.WORD, background="black", foreground="green")
        self.terminal.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Input frame
        input_frame = ttk.Frame(self.root)
        input_frame.pack(fill=tk.X, padx=10, pady=5)
        
        self.input_entry = ttk.Entry(input_frame)
        self.input_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)
        self.input_entry.bind("<Return>", self.send_message)
        
        self.send_button = ttk.Button(input_frame, text="Send", command=self.send_message)
        self.send_button.pack(side=tk.RIGHT, padx=5)
        
        # Buttons frame
        buttons_frame = ttk.Frame(self.root)
        buttons_frame.pack(fill=tk.X, padx=10, pady=5)
        
        # Command buttons
        self.led_on_button = ttk.Button(buttons_frame, text="LED ON", command=lambda: self.send_command("LED ON"))
        self.led_on_button.pack(side=tk.LEFT, padx=5)
        
        self.led_off_button = ttk.Button(buttons_frame, text="LED OFF", command=lambda: self.send_command("LED OFF"))
        self.led_off_button.pack(side=tk.LEFT, padx=5)
        
        self.status_button = ttk.Button(buttons_frame, text="STATUS", command=lambda: self.send_command("STATUS"))
        self.status_button.pack(side=tk.LEFT, padx=5)
        
        self.dump_log_button = ttk.Button(buttons_frame, text="DUMP LOG", command=lambda: self.send_command("DUMP LOG"))
        self.dump_log_button.pack(side=tk.LEFT, padx=5)
        
        # Control buttons frame
        control_frame = ttk.Frame(self.root)
        control_frame.pack(fill=tk.X, padx=10, pady=5)
        
        self.clear_button = ttk.Button(control_frame, text="Clear Terminal", command=self.clear_terminal)
        self.clear_button.pack(side=tk.LEFT, padx=5)
        
        # Newline option
        self.newline_var = tk.StringVar(value="\\n")
        ttk.Label(control_frame, text="Newline:").pack(side=tk.LEFT, padx=(20, 5))
        
        self.newline_combo = ttk.Combobox(control_frame, textvariable=self.newline_var, 
                                          values=["\\r\\n", "\\n", "\\r", "None"], width=5)
        self.newline_combo.pack(side=tk.LEFT)
        
        # Add delay setting button
        self.delay_button = ttk.Button(control_frame, text=f"TX Delay: {self.tx_delay}s", 
                                      command=self.set_tx_delay)
        self.delay_button.pack(side=tk.LEFT, padx=(20, 5))
        
    def set_tx_delay(self):
        """Set the transmission delay"""
        new_delay = simpledialog.askfloat("Set TX Delay", 
                                         "Enter transmission delay (seconds):", 
                                         minvalue=0.0, maxvalue=5.0,
                                         initialvalue=self.tx_delay)
        if new_delay is not None:
            self.tx_delay = new_delay
            self.delay_button.config(text=f"TX Delay: {self.tx_delay}s")
            self.terminal.insert(tk.END, f"TX delay set to {self.tx_delay} seconds\n")
            self.terminal.see(tk.END)
        
    def port_refresh(self):
        """Refresh available serial ports"""
        import serial.tools.list_ports
        
        self.port_combobox["values"] = [port.device for port in serial.tools.list_ports.comports()]
        if len(self.port_combobox["values"]) > 0:
            self.port_combobox.current(0)
            
    def toggle_connection(self):
        """Connect or disconnect from serial port"""
        if not self.is_connected:
            try:
                port = self.port_combobox.get()
                baud = int(self.baud_combobox.get())
                
                self.serial_port = serial.Serial(port, baud, timeout=0.1)
                self.is_connected = True
                self.connect_button.config(text="Disconnect")
                
                self.terminal.insert(tk.END, f"Connected to {port} at {baud} baud\n")
                self.terminal.see(tk.END)
                
                # Start receiving thread
                self.stop_thread = False
                self.receiving_thread = threading.Thread(target=self.receive_data)
                self.receiving_thread.daemon = True
                self.receiving_thread.start()
                
            except Exception as e:
                self.terminal.insert(tk.END, f"Connection error: {str(e)}\n")
                self.terminal.see(tk.END)
        else:
            # Disconnect
            self.stop_thread = True
            if self.receiving_thread:
                self.receiving_thread.join(timeout=1.0)
            
            if self.serial_port:
                self.serial_port.close()
                self.serial_port = None
                
            self.is_connected = False
            self.connect_button.config(text="Connect")
            
            self.terminal.insert(tk.END, "Disconnected\n")
            self.terminal.see(tk.END)
            
    def receive_data(self):
        """Thread function to continuously receive data"""
        while not self.stop_thread and self.serial_port:
            try:
                if self.serial_port.in_waiting > 0:
                    data = self.serial_port.read(self.serial_port.in_waiting)
                    if data:
                        # Using a lambda to update the UI from the thread
                        self.root.after(0, lambda d=data: self.update_terminal(d))
            except Exception as e:
                self.root.after(0, lambda: self.terminal.insert(tk.END, f"Error reading: {str(e)}\n"))
                break
                
            time.sleep(0.05)  # Small delay to prevent CPU hogging
            
    def update_terminal(self, data):
        """Update terminal with received data"""
        text = data.decode('utf-8', errors='replace')
        self.terminal.insert(tk.END, text)
        self.terminal.see(tk.END)
        
    def send_message(self, event=None):
        """Send message to STM32"""
        if not self.is_connected or not self.serial_port:
            self.terminal.insert(tk.END, "Not connected\n")
            self.terminal.see(tk.END)
            return
            
        message = self.input_entry.get()
        if not message:
            return
            
        self.send_raw_message(message)
        self.input_entry.delete(0, tk.END)
        
    def send_command(self, command):
        """Send a predefined command"""
        if self.is_connected and self.serial_port:
            self.send_raw_message(command)
        else:
            self.terminal.insert(tk.END, "Not connected\n")
            self.terminal.see(tk.END)
            
    def send_raw_message(self, message):
        """Send raw message with appropriate newline"""
        try:
            # Add newline if needed
            newline = self.newline_var.get()
            if newline != "None":
                nl = newline.encode().decode('unicode_escape')
                full_message = message + nl
            else:
                full_message = message
                
            # Display what we're sending
            self.terminal.insert(tk.END, f"> {message}\n")
            self.terminal.see(tk.END)
            
            # Send the message - one character at a time with small delays
            for char in full_message:
                self.serial_port.write(char.encode('utf-8'))
                self.serial_port.flush()  # Ensure data is sent immediately
                time.sleep(0.001)  # 1ms delay between characters
            
            # Add overall delay after sending complete message
            time.sleep(self.tx_delay)
            
        except Exception as e:
            self.terminal.insert(tk.END, f"Send error: {str(e)}\n")
            self.terminal.see(tk.END)
        
    def clear_terminal(self):
        """Clear the terminal"""
        self.terminal.delete(1.0, tk.END)
        
if __name__ == "__main__":
    root = tk.Tk()
    app = STM32CommunicationApp(root)
    root.mainloop()