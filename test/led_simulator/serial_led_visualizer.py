#!/usr/bin/env python3
"""
Serial LED Visualizer - Reads LED data from Arduino and displays it in real-time.
"""

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import serial
import time
import argparse
import re
import sys
import os

# Default constants (will be updated from serial data)
NUM_STRIPS = 7
LEDS_PER_STRIP = 144
NUM_PIXELS = NUM_STRIPS * LEDS_PER_STRIP

class SerialLEDVisualizer:
    def __init__(self, port, baud_rate=921600):
        self.port = port
        self.baud_rate = baud_rate
        self.serial_connection = None
        self.led_colors = np.zeros((NUM_PIXELS, 3))
        self.strip_layout = None
        self.fig = None
        self.ax = None
        self.led_display = None
        self.animation = None
        self.receiving_data = False
        self.current_frame = []

    def connect_serial(self):
        """Connect to the serial port"""
        try:
            self.serial_connection = serial.Serial(self.port, self.baud_rate, timeout=1)
            print(f"Connected to {self.port} at {self.baud_rate} baud")
            return True
        except Exception as e:
            print(f"Error connecting to serial port: {e}")
            return False

    def parse_led_data(self, line):
        """Parse a line of LED data from the serial port"""
        # Check for start/end markers
        if line.startswith("LED_DATA_START"):
            self.receiving_data = True
            self.current_frame = []
            return

        if line.startswith("LED_DATA_END"):
            self.receiving_data = False
            return

        # Parse configuration data
        config_match = re.match(r"NUM_STRIPS:(\d+),LEDS_PER_STRIP:(\d+)", line)
        if config_match:
            global NUM_STRIPS, LEDS_PER_STRIP, NUM_PIXELS
            NUM_STRIPS = int(config_match.group(1))
            LEDS_PER_STRIP = int(config_match.group(2))
            NUM_PIXELS = NUM_STRIPS * LEDS_PER_STRIP

            # Resize the LED colors array if needed
            if self.led_colors.shape[0] != NUM_PIXELS:
                self.led_colors = np.zeros((NUM_PIXELS, 3))

                # Update the display layout
                if self.strip_layout is not None:
                    self.update_layout()
            return

        # Parse LED data
        led_match = re.match(r"LED:(\d+),(\d+),(\d+),(\d+)", line)
        if led_match and self.receiving_data:
            led_index = int(led_match.group(1))
            r = int(led_match.group(2))
            g = int(led_match.group(3))
            b = int(led_match.group(4))

            if led_index < NUM_PIXELS:
                # Normalize to 0-1 range for matplotlib
                self.led_colors[led_index, 0] = r / 255.0
                self.led_colors[led_index, 1] = g / 255.0
                self.led_colors[led_index, 2] = b / 255.0

    def update_layout(self):
        """Update the visualization layout based on current strip configuration"""
        # Determine the best layout for the strips
        if NUM_STRIPS == 1:
            # Single strip - show as one row
            self.strip_layout = self.led_colors.reshape(1, NUM_PIXELS, 3)
        else:
            # Multiple strips - show as multiple rows
            self.strip_layout = self.led_colors.reshape(NUM_STRIPS, LEDS_PER_STRIP, 3)

        # Update the display if it exists
        if self.led_display is not None:
            self.led_display.set_array(self.strip_layout)

    def read_serial_data(self):
        """Read data from the serial port"""
        if self.serial_connection is None or not self.serial_connection.is_open:
            return

        # Read all available lines
        while self.serial_connection.in_waiting:
            try:
                line = self.serial_connection.readline().decode('utf-8').strip()
                if line:
                    self.parse_led_data(line)
            except Exception as e:
                print(f"Error reading serial data: {e}")
                break

    def update_animation(self, frame):
        """Update function for the animation"""
        self.read_serial_data()

        # Update the layout if needed
        if self.strip_layout is None:
            self.update_layout()

        # Update the display
        self.led_display.set_array(self.strip_layout)
        return self.led_display,

    def setup_visualization(self):
        """Set up the matplotlib visualization"""
        # Create figure and axis
        self.fig, self.ax = plt.subplots(figsize=(12, NUM_STRIPS * 2))
        plt.subplots_adjust(left=0.05, right=0.95, top=0.9, bottom=0.1)

        # Create title
        plt.title('LED Strip Visualizer - Reading from Serial\nClose window to exit')

        # Initialize the layout
        self.update_layout()

        # Create the display
        self.led_display = self.ax.imshow(self.strip_layout, aspect='auto', interpolation='nearest')

        # Remove axis ticks
        self.ax.set_xticks([])
        self.ax.set_yticks([])

        # Add strip labels if multiple strips
        if NUM_STRIPS > 1:
            self.ax.set_yticks(np.arange(NUM_STRIPS))
            self.ax.set_yticklabels([f"Strip {i+1}" for i in range(NUM_STRIPS)])

    def start(self):
        """Start the visualization"""
        if not self.connect_serial():
            return

        self.setup_visualization()

        # Create animation
        self.animation = FuncAnimation(
            self.fig, self.update_animation,
            frames=None, interval=33, blit=True
        )

        # Show the plot
        plt.show()

        # Clean up when the window is closed
        if self.serial_connection and self.serial_connection.is_open:
            self.serial_connection.close()
            print("Serial connection closed")

def list_serial_ports():
    """List available serial ports"""
    import serial.tools.list_ports
    ports = serial.tools.list_ports.comports()

    if not ports:
        print("No serial ports found.")
        return

    print("Available serial ports:")
    for port in ports:
        print(f"  {port.device} - {port.description}")

def main():
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Serial LED Visualizer')
    parser.add_argument('port', nargs='?', help='Serial port (e.g., COM3 on Windows or /dev/ttyUSB0 on Linux/Mac)')
    parser.add_argument('--baud', type=int, default=921600, help='Baud rate (default: 921600)')
    parser.add_argument('--list', action='store_true', help='List available serial ports')
    args = parser.parse_args()

    # List available ports if requested
    if args.list:
        list_serial_ports()
        return

    # Check if port is provided
    if not args.port:
        print("Error: Serial port is required.")
        print("Use --list to see available ports.")
        print(f"Usage: {os.path.basename(sys.argv[0])} <serial_port> [--baud BAUD_RATE]")
        return

    # Create and start the visualizer
    try:
        visualizer = SerialLEDVisualizer(args.port, args.baud)
        visualizer.start()
    except KeyboardInterrupt:
        print("\nVisualization stopped by user.")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()
