import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import matplotlib.colors as mcolors
import time
import math

# Constants from the original code
NUM_STRIPS = 1  # Just showing one strip for simplicity
LEDS_PER_STRIP = 144
NUM_PIXELS = NUM_STRIPS * LEDS_PER_STRIP

# Speed limits in units led/s
SPEED_MIN = 0.0
SPEED_MAX = 0.5

# Wavelength limits in units leds
WAVELN_MIN = 0.0
WAVELN_MAX = 400.0

# Brightness limits
BRIGHTNESS_MIN = 0.0
BRIGHTNESS_MAX = 255.0

# Initial parameters
red_params = {'speed': 0.1, 'wavelength': 100, 'brightness': 128, 'phase': 0}
green_params = {'speed': 0.15, 'wavelength': 150, 'brightness': 128, 'phase': 1000000}
blue_params = {'speed': 0.2, 'wavelength': 200, 'brightness': 128, 'phase': 2000000}

# Function to calculate color based on position and time
def calc_color(i, t, speed, wavelength, brightness, phase=0):
    if wavelength == 0:
        # If wavelength is 0, just use speed to calculate position
        pos = speed * (t - phase) / 1000000.0
    else:
        pos = (i / wavelength) + (speed * (t - phase) / 1000000.0)

    # Apply sine wave transformation
    pos = 0.5 * (1.0 + math.sin(2 * math.pi * pos))
    # Apply brightness
    pos = pos * brightness

    return int(pos)

# Set up the figure and axis
fig, ax = plt.subplots(figsize=(12, 3))
plt.subplots_adjust(left=0.05, right=0.95, top=0.9, bottom=0.2)

# Create a title with instructions
plt.title('LED Strip Simulator\nClose window to exit')

# Create initial empty plot
led_colors = np.zeros((LEDS_PER_STRIP, 3))
led_display = ax.imshow(led_colors.reshape(1, -1, 3), aspect='auto', interpolation='nearest')

# Remove axis ticks
ax.set_xticks([])
ax.set_yticks([])

# Add sliders for controlling parameters
from matplotlib.widgets import Slider

# Red sliders
ax_red_speed = plt.axes([0.1, 0.15, 0.2, 0.03])
ax_red_wavelength = plt.axes([0.1, 0.1, 0.2, 0.03])
ax_red_brightness = plt.axes([0.1, 0.05, 0.2, 0.03])

slider_red_speed = Slider(ax_red_speed, 'Red Speed', 0, 1, valinit=red_params['speed']/SPEED_MAX)
slider_red_wavelength = Slider(ax_red_wavelength, 'Red Wavelength', 0, 1, valinit=red_params['wavelength']/WAVELN_MAX)
slider_red_brightness = Slider(ax_red_brightness, 'Red Brightness', 0, 1, valinit=red_params['brightness']/BRIGHTNESS_MAX)

# Green sliders
ax_green_speed = plt.axes([0.4, 0.15, 0.2, 0.03])
ax_green_wavelength = plt.axes([0.4, 0.1, 0.2, 0.03])
ax_green_brightness = plt.axes([0.4, 0.05, 0.2, 0.03])

slider_green_speed = Slider(ax_green_speed, 'Green Speed', 0, 1, valinit=green_params['speed']/SPEED_MAX)
slider_green_wavelength = Slider(ax_green_wavelength, 'Green Wavelength', 0, 1, valinit=green_params['wavelength']/WAVELN_MAX)
slider_green_brightness = Slider(ax_green_brightness, 'Green Brightness', 0, 1, valinit=green_params['brightness']/BRIGHTNESS_MAX)

# Blue sliders
ax_blue_speed = plt.axes([0.7, 0.15, 0.2, 0.03])
ax_blue_wavelength = plt.axes([0.7, 0.1, 0.2, 0.03])
ax_blue_brightness = plt.axes([0.7, 0.05, 0.2, 0.03])

slider_blue_speed = Slider(ax_blue_speed, 'Blue Speed', 0, 1, valinit=blue_params['speed']/SPEED_MAX)
slider_blue_wavelength = Slider(ax_blue_wavelength, 'Blue Wavelength', 0, 1, valinit=blue_params['wavelength']/WAVELN_MAX)
slider_blue_brightness = Slider(ax_blue_brightness, 'Blue Brightness', 0, 1, valinit=blue_params['brightness']/BRIGHTNESS_MAX)

# Update function for the sliders
def update_sliders(val):
    red_params['speed'] = slider_red_speed.val * SPEED_MAX
    red_params['wavelength'] = slider_red_wavelength.val * WAVELN_MAX
    red_params['brightness'] = slider_red_brightness.val * BRIGHTNESS_MAX

    green_params['speed'] = slider_green_speed.val * SPEED_MAX
    green_params['wavelength'] = slider_green_wavelength.val * WAVELN_MAX
    green_params['brightness'] = slider_green_brightness.val * BRIGHTNESS_MAX

    blue_params['speed'] = slider_blue_speed.val * SPEED_MAX
    blue_params['wavelength'] = slider_blue_wavelength.val * WAVELN_MAX
    blue_params['brightness'] = slider_blue_brightness.val * BRIGHTNESS_MAX

# Connect the sliders to the update function
slider_red_speed.on_changed(update_sliders)
slider_red_wavelength.on_changed(update_sliders)
slider_red_brightness.on_changed(update_sliders)
slider_green_speed.on_changed(update_sliders)
slider_green_wavelength.on_changed(update_sliders)
slider_green_brightness.on_changed(update_sliders)
slider_blue_speed.on_changed(update_sliders)
slider_blue_wavelength.on_changed(update_sliders)
slider_blue_brightness.on_changed(update_sliders)

# Animation update function
def update(frame):
    # Current time in microseconds
    t = int(time.time() * 1000000)

    # Update LED colors
    for i in range(LEDS_PER_STRIP):
        r = calc_color(i, t, red_params['speed'], red_params['wavelength'], red_params['brightness'], red_params['phase'])
        g = calc_color(i, t, green_params['speed'], green_params['wavelength'], green_params['brightness'], green_params['phase'])
        b = calc_color(i, t, blue_params['speed'], blue_params['wavelength'], blue_params['brightness'], blue_params['phase'])

        # Normalize to 0-1 range for matplotlib
        led_colors[i, 0] = r / 255.0
        led_colors[i, 1] = g / 255.0
        led_colors[i, 2] = b / 255.0

    # Update the display
    led_display.set_array(led_colors.reshape(1, -1, 3))
    return led_display,

# Create animation
ani = FuncAnimation(fig, update, frames=None, interval=33, blit=True)

# Show the plot
plt.show()
