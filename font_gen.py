

import freetype
import numpy as np
import os
import matplotlib.pyplot as plt
import time
import math

# Define font settings
FONT_PATH = "UbuntuNerdFontPropo_Regular.ttf"  # Replace with actual TTF file path
FONT_SIZE = 64 # Adjust as needed
FONT_NAME = FONT_PATH.split('.')[0] + str(FONT_SIZE) + "pt"
OUTPUT_FILE =  FONT_NAME + ".h"

# ASCII Range 0x20 (Space) to 0x7E (~)
CHARACTERS = [chr(i) for i in range(0x20, 0x7F)]

def visualize_glyph(data, char):
    if data.size == 0 or data.shape[1] == 0:  # Skip empty glyphs
        print(f"Skipping visualization for '{char}' (empty glyph)")
        return
    """Display the character glyph as an image for debugging."""
    plt.imshow(data, cmap="gray_r")
    plt.title(f"Glyph: {char}")
    plt.axis("off")
    plt.show(block=True)  # Show without blocking execution
    # time.sleep(1)  # Short pause to inspect the rendering


def render_char(font, char):
    """Render a character to a monochrome bitmap and ensure correct alignment."""
    font.load_char(char, freetype.FT_LOAD_RENDER)
    bitmap = font.glyph.bitmap
    width = bitmap.width
    height = bitmap.rows+1
    pitch = bitmap.pitch  # Bytes per row in raw bitmap buffer

    # Ensure each row is byte-aligned (pad to the nearest 8 pixels)
    padded_width = ((width + 7) // 8) * 8  # Ensure row can fit into full bytes

    # Create a blank data array (binary)
    data = np.zeros((height, padded_width), dtype=np.uint8)

    # Ensure `bitmap.buffer` is always in bytes format
    buffer_data = bytes(bitmap.buffer)
    buffer_array = np.frombuffer(buffer_data, dtype=np.uint8)

    for y in range(height):
        row_start = y * pitch  # Correct row start
        row_data = buffer_array[row_start:row_start + width]  # Correct slicing
        data[y, :len(row_data)] = row_data  # Copy actual glyph pixels

    return data[:, :width], width, height  # Return only the valid width



def generate_font_header():
    """Generate a .h file for Adafruit GFX from TTF."""
    font = freetype.Face(FONT_PATH)
    font.set_pixel_sizes(0, FONT_SIZE)

    bitmap_data = []
    char_map = {}
    offsets = []
    current_offset = 0

    max_alnum_height = 0
    min_lower_height = 999

    # Find the tallest alphanumeric character (A-Z, a-z, 0-9)
    for char in "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789":
        _, _, height = render_char(font, char)
        max_alnum_height = max(max_alnum_height, height)
        max_alnum_height_char = char
    for char in "abcdefhiklmnorstuvwxz":
        _, _, height = render_char(font, char)
        min_lower_height = min(min_lower_height, height)
        min_lower_height_char = char
    o_height = render_char(font, "o")[2]
    i_height = render_char(font, "i")[2]

    print(f"Tallest alphanumeric character height: {char} {max_alnum_height}")
    print(f"Shortest lowercase character height: {min_lower_height_char} {min_lower_height}")

    for char in CHARACTERS:
        data, width, height = render_char(font, char)
        char_map[char] = (width, height)
        # visualize_glyph(data, char)
        # Convert bitmap to hex format
        byte_data = []
        for row in data:
            actual_width_bits = width  # The real width of the character in bits
            padded_width_bits = (actual_width_bits + 7) // 8 * 8  # Round up to the nearest 8

            # Pad row to a full byte width to prevent misalignment
            row_padded = np.pad(row, (0, padded_width_bits - actual_width_bits), mode='constant')

            # Convert to packed byte format
            row_packed = np.packbits(row_padded, bitorder="big")  # Ensure MSB-first (Adafruit format)
            byte_data.extend(row_packed)

        bitmap_data.append((char, byte_data))
        offsets.append(current_offset)
        current_offset += len(byte_data)  # Update offset for next character

    # Write the .h file
    with open(OUTPUT_FILE, "w") as f:
        f.write("#pragma once\n#include <Adafruit_GFX.h>\n\n")
        f.write(f"const uint8_t {FONT_NAME}Bitmaps[]" + " PROGMEM = {\n")

        # Write bitmap data
        for char, byte_data in bitmap_data:
            print(len(byte_data))
            if len(byte_data) != 0:
                f.write(f"    /* {char} */ ")
                f.write(", ".join(f"0x{b:02X}" for b in byte_data) + ",\n")
            else:
                f.write(f"    /* {char} */ \n")

        f.write("};\n\n")

        # Define character descriptor table
        f.write(f"const GFXglyph {FONT_NAME}Glyphs[]" + " PROGMEM = {\n")
        for i, (char, (width, height)) in enumerate(char_map.items()):
            if char in "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789":
                yOff = int(-max_alnum_height*0.8)
            elif char in "gpqy":
                yOff = int(-o_height)
            elif char in "j":
                yOff = int(-i_height)
            elif char in "abcdefhiklmnorstuvwxz":
                yOff = int(-height) 
            elif char in ":;<>=~*+-":
                yOff = int(-(max_alnum_height+height)*0.45)
            else:
                yOff = -height
            f.write(f"    {{ {offsets[i]}, {math.ceil(width/8)*8}, {height}, {width+1}, 0, {yOff} }},  // {char}\n")

        f.write("};\n\n")

        # Font struct
        f.write(f"const GFXfont {FONT_NAME}" + " PROGMEM = {\n")
        f.write(f"    (uint8_t*){FONT_NAME}Bitmaps,\n")
        f.write(f"    (GFXglyph*){FONT_NAME}Glyphs,\n")
        f.write(f"    0x20, 0x7E, {FONT_SIZE}\n")
        f.write("};\n")

    print(f"Header file generated: {OUTPUT_FILE}")


# Run the script
generate_font_header()
