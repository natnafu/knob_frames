import tkinter as tk
import math
import time

# Create a simple window
root = tk.Tk()
root.title("Simple LED Strip Simulator")
root.geometry("1200x400")

# Create a canvas to draw on
canvas = tk.Canvas(root, bg="black", width=1200, height=300)
canvas.pack(pady=20)

# Number of LEDs
num_leds = 144

# LED width
led_width = 1200 // num_leds

# Function to calculate color based on position and time
def calc_color(pos, t, speed, wavelength, brightness, phase=0):
    if wavelength == 0:
        # If wavelength is 0, just use speed to calculate position
        pos_val = speed * (t - phase) / 1000000.0
    else:
        pos_val = (pos / wavelength) + (speed * (t - phase) / 1000000.0)

    # Apply sine wave transformation
    pos_val = 0.5 * (1.0 + math.sin(2 * math.pi * pos_val))
    # Apply brightness
    pos_val = pos_val * brightness

    return int(pos_val)

# Animation function
def update():
    # Clear canvas
    canvas.delete("all")

    # Current time in microseconds
    t = int(time.time() * 1000000)

    # Draw LEDs
    for i in range(num_leds):
        # Calculate RGB values with different parameters for each color
        r = calc_color(i, t, 0.1, 100, 128)
        g = calc_color(i, t, 0.15, 150, 128, 1000000)
        b = calc_color(i, t, 0.2, 200, 128, 2000000)

        # Convert to hex color
        color = f'#{r:02x}{g:02x}{b:02x}'

        # Draw LED
        canvas.create_rectangle(
            i * led_width, 0,
            (i + 1) * led_width - 1, 300,
            fill=color, outline=""
        )

    # Schedule next update
    root.after(33, update)  # ~30 FPS

# Start animation
update()

# Run the application
root.mainloop()
