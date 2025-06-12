import tkinter as tk
from tkinter import ttk
import numpy as np
import time
import math

# Constants from the original code
NUM_STRIPS = 5
LEDS_PER_STRIP = 144
NUM_PIXELS = NUM_STRIPS * LEDS_PER_STRIP

# Speed limits in units led/s
SPEED_MIN = 0.0
SPEED_MAX = 0.5
SPEED_CHANGE_THRESHOLD = 0.02 * (SPEED_MAX - SPEED_MIN)
SPEED_SMOOTHING_FACTOR = 0.1

# Wavelength limits in units leds
WAVELN_MIN = 0.0
WAVELN_MAX = 400.0
WAVELN_THRESHOLD = 3.0  # only change by whole number of leds
WAVELN_SMOOTHING_FACTOR = 0.1

# Brightness limits
BRIGHTNESS_MIN = 0.0
BRIGHTNESS_MAX = 255.0
BRIGHT_CHANGE_THRESHOLD = 0.01 * (BRIGHTNESS_MAX - BRIGHTNESS_MIN)
BRIGHT_SMOOTHING_FACTOR = 0.1

# LED display settings
LED_SIZE = 5  # Size of each LED in pixels
LED_SPACING = 1  # Spacing between LEDs in pixels
DISPLAY_LEDS = 144  # Number of LEDs to display (showing just one strip for clarity)

class Color:
    def __init__(self, name):
        self.name = name
        # Wave parameters
        self.speed_current = 0
        self.speed_target = 0
        self.waveln_current = 0
        self.waveln_target = 0
        self.brightness_current = 0
        self.brightness_target = 0
        # Phase offset in microseconds
        self.phase_us = 0

class LEDStripSimulator:
    def __init__(self, root):
        self.root = root
        self.root.title("LED Strip Simulator")
        self.root.geometry("1200x700")
        self.root.resizable(True, True)

        # Create color objects
        self.red = Color("Red")
        self.green = Color("Green")
        self.blue = Color("Blue")

        # Initialize with some default values to make it visually interesting from the start
        self.red.speed_current = 0.1
        self.red.speed_target = 0.1
        self.red.waveln_current = 100
        self.red.waveln_target = 100
        self.red.brightness_current = 128
        self.red.brightness_target = 128

        self.green.speed_current = 0.15
        self.green.speed_target = 0.15
        self.green.waveln_current = 150
        self.green.waveln_target = 150
        self.green.brightness_current = 128
        self.green.brightness_target = 128

        self.blue.speed_current = 0.2
        self.blue.speed_target = 0.2
        self.blue.waveln_current = 200
        self.blue.waveln_target = 200
        self.blue.brightness_current = 128
        self.blue.brightness_target = 128

        # Set up the UI
        self.setup_ui()

        # Initialize time variables
        self.start_time = time.time() * 1000000  # microseconds

        # Start the animation
        self.running = True
        self.update_and_draw()

    def setup_ui(self):
        # Create frames
        control_frame = ttk.Frame(self.root, padding="10")
        control_frame.pack(side=tk.TOP, fill=tk.X)

        display_frame = ttk.Frame(self.root, padding="10")
        display_frame.pack(side=tk.BOTTOM, fill=tk.BOTH, expand=True)

        # Create LED display frame
        self.led_frame = ttk.Frame(display_frame, padding="5")
        self.led_frame.pack(fill=tk.BOTH, expand=True)
        self.led_frame.configure(style="Black.TFrame")

        # Create a style for the black background
        style = ttk.Style()
        style.configure("Black.TFrame", background="black")

        # Create LED labels
        self.leds = []
        for i in range(DISPLAY_LEDS):
            led = tk.Label(self.led_frame, width=1, height=20, bg="black")
            led.grid(row=0, column=i, padx=0, pady=0, sticky="nsew")
            self.leds.append(led)

        # Configure grid to expand LEDs evenly
        for i in range(DISPLAY_LEDS):
            self.led_frame.columnconfigure(i, weight=1)
        self.led_frame.rowconfigure(0, weight=1)

        # Create control panels for each color
        self.create_color_controls(control_frame, "Red", self.red, "#FF0000")
        self.create_color_controls(control_frame, "Green", self.green, "#00FF00")
        self.create_color_controls(control_frame, "Blue", self.blue, "#0000FF")

        # Create reset button
        reset_button = ttk.Button(control_frame, text="Reset All", command=self.reset_all)
        reset_button.pack(side=tk.RIGHT, padx=10)

    def create_color_controls(self, parent, color_name, color_obj, color_hex):
        frame = ttk.LabelFrame(parent, text=color_name, padding="5")
        frame.pack(side=tk.LEFT, fill=tk.Y, expand=True, padx=5)

        # Speed control
        ttk.Label(frame, text="Speed:").grid(row=0, column=0, sticky=tk.W, padx=5, pady=2)
        # Set initial values based on the color object's current values
        initial_speed = (color_obj.speed_current - SPEED_MIN) / (SPEED_MAX - SPEED_MIN)
        speed_var = tk.DoubleVar(value=initial_speed)
        speed_slider = ttk.Scale(frame, from_=0, to=1, orient=tk.HORIZONTAL,
                                variable=speed_var, length=200)
        speed_slider.grid(row=0, column=1, sticky=tk.EW, padx=5, pady=2)
        speed_slider.bind("<B1-Motion>", lambda e, v=speed_var, c=color_obj: self.update_speed(v, c))
        speed_slider.bind("<ButtonRelease-1>", lambda e, v=speed_var, c=color_obj: self.update_speed(v, c))

        # Wavelength control
        ttk.Label(frame, text="Wavelength:").grid(row=1, column=0, sticky=tk.W, padx=5, pady=2)
        initial_waveln = (color_obj.waveln_current - WAVELN_MIN) / (WAVELN_MAX - WAVELN_MIN)
        waveln_var = tk.DoubleVar(value=initial_waveln)
        waveln_slider = ttk.Scale(frame, from_=0, to=1, orient=tk.HORIZONTAL,
                                 variable=waveln_var, length=200)
        waveln_slider.grid(row=1, column=1, sticky=tk.EW, padx=5, pady=2)
        waveln_slider.bind("<B1-Motion>", lambda e, v=waveln_var, c=color_obj: self.update_wavelength(v, c))
        waveln_slider.bind("<ButtonRelease-1>", lambda e, v=waveln_var, c=color_obj: self.update_wavelength(v, c))

        # Brightness control
        ttk.Label(frame, text="Brightness:").grid(row=2, column=0, sticky=tk.W, padx=5, pady=2)
        initial_bright = (color_obj.brightness_current - BRIGHTNESS_MIN) / (BRIGHTNESS_MAX - BRIGHTNESS_MIN)
        bright_var = tk.DoubleVar(value=initial_bright)
        bright_slider = ttk.Scale(frame, from_=0, to=1, orient=tk.HORIZONTAL,
                                 variable=bright_var, length=200)
        bright_slider.grid(row=2, column=1, sticky=tk.EW, padx=5, pady=2)
        bright_slider.bind("<B1-Motion>", lambda e, v=bright_var, c=color_obj: self.update_brightness(v, c))
        bright_slider.bind("<ButtonRelease-1>", lambda e, v=bright_var, c=color_obj: self.update_brightness(v, c))

        # Value labels
        speed_label = ttk.Label(frame, text="0.00")
        speed_label.grid(row=0, column=2, padx=5, pady=2)

        waveln_label = ttk.Label(frame, text="0.00")
        waveln_label.grid(row=1, column=2, padx=5, pady=2)

        bright_label = ttk.Label(frame, text="0.00")
        bright_label.grid(row=2, column=2, padx=5, pady=2)

        # Store references to update labels
        color_obj.speed_label = speed_label
        color_obj.waveln_label = waveln_label
        color_obj.bright_label = bright_label
        color_obj.speed_var = speed_var
        color_obj.waveln_var = waveln_var
        color_obj.bright_var = bright_var

    def update_speed(self, var, color):
        new_target = var.get() * (SPEED_MAX - SPEED_MIN) + SPEED_MIN
        color.speed_target = self.apply_hysteresis(new_target, color.speed_target, SPEED_CHANGE_THRESHOLD)
        color.speed_current = self.apply_smoothing(color.speed_target, color.speed_current, SPEED_SMOOTHING_FACTOR)
        color.speed_label.config(text=f"{color.speed_current:.2f}")

    def update_wavelength(self, var, color):
        new_target = var.get() * (WAVELN_MAX - WAVELN_MIN) + WAVELN_MIN
        color.waveln_target = self.apply_hysteresis(new_target, color.waveln_target, WAVELN_THRESHOLD)
        color.waveln_current = self.apply_smoothing(color.waveln_target, color.waveln_current, WAVELN_SMOOTHING_FACTOR)
        color.waveln_label.config(text=f"{color.waveln_current:.2f}")

    def update_brightness(self, var, color):
        new_target = var.get() * (BRIGHTNESS_MAX - BRIGHTNESS_MIN) + BRIGHTNESS_MIN
        color.brightness_target = self.apply_hysteresis(new_target, color.brightness_target, BRIGHT_CHANGE_THRESHOLD)
        color.brightness_current = self.apply_smoothing(color.brightness_target, color.brightness_current, BRIGHT_SMOOTHING_FACTOR)
        color.bright_label.config(text=f"{color.brightness_current:.2f}")

    def apply_hysteresis(self, new_value, last_value, threshold):
        if abs(new_value - last_value) < threshold:
            return last_value  # no significant change
        return new_value  # significant change

    def apply_smoothing(self, target_value, current_value, smoothing_factor):
        return current_value + (target_value - current_value) * smoothing_factor

    def calc_color(self, color, i, t):
        if color.waveln_current == 0:
            # If wavelength is 0, just use speed to calculate position
            pos = color.speed_current * (t - color.phase_us) / 1000000.0
        else:
            pos = (i / color.waveln_current) + (color.speed_current * (t - color.phase_us) / 1000000.0)

        # Apply sine wave transformation
        pos = 0.5 * (1.0 + math.sin(2 * math.pi * pos))
        # Apply brightness
        pos = pos * color.brightness_current

        return int(pos)

    def reset_all(self):
        # Reset all sliders and values
        for color in [self.red, self.green, self.blue]:
            color.speed_var.set(0)
            color.waveln_var.set(0)
            color.bright_var.set(0)
            color.speed_current = 0
            color.speed_target = 0
            color.waveln_current = 0
            color.waveln_target = 0
            color.brightness_current = 0
            color.brightness_target = 0
            color.speed_label.config(text="0.00")
            color.waveln_label.config(text="0.00")
            color.bright_label.config(text="0.00")

    def update_parameters(self):
        # Update all parameters based on current slider values
        self.update_speed(self.red.speed_var, self.red)
        self.update_wavelength(self.red.waveln_var, self.red)
        self.update_brightness(self.red.bright_var, self.red)

        self.update_speed(self.green.speed_var, self.green)
        self.update_wavelength(self.green.waveln_var, self.green)
        self.update_brightness(self.green.bright_var, self.green)

        self.update_speed(self.blue.speed_var, self.blue)
        self.update_wavelength(self.blue.waveln_var, self.blue)
        self.update_brightness(self.blue.bright_var, self.blue)

    def draw_leds(self):
        # Current time in microseconds
        t = int(time.time() * 1000000)

        # Update each LED
        for i in range(DISPLAY_LEDS):
            # Calculate RGB values
            r = self.calc_color(self.red, i, t)
            g = self.calc_color(self.green, i, t)
            b = self.calc_color(self.blue, i, t)

            # Convert to hex color
            color = f'#{r:02x}{g:02x}{b:02x}'

            # Update LED color
            self.leds[i].config(bg=color)

    def update_and_draw(self):
        if not self.running:
            return

        try:
            # Update parameters
            self.update_parameters()

            # Draw LEDs
            self.draw_leds()

            # Schedule the next update
            self.root.after(33, self.update_and_draw)  # ~30 FPS
        except Exception as e:
            print(f"Error in animation: {e}")

    def on_closing(self):
        self.running = False
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = LEDStripSimulator(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()
