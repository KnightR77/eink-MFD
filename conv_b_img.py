from PIL import Image

# Load image
img = Image.open("input.png")

# Convert to 1-bit and resize to match your display (example: 128x250)
img = img.convert("1")
img = img.resize((30, 30))  # Change to your display's resolution

# --- Show the final preview ---
img.show(title="GxEPD Preview - 1-bit BW")

# Convert to GxEPD-style byte array
def convert_to_gxepd_array(img):
    data = []
    width, height = img.size
    for y in range(0, height, 8):
        for x in range(width):
            byte = 0
            for b in range(8):
                if y + b < height:
                    pixel = img.getpixel((x, y + b))
                    if pixel == 0:  # Black pixel
                        byte |= (1 << b)
            data.append(byte)
    return data

def convert_to_gfx_bitmap(img):
    img = img.convert("1")
    width, height = img.size
    data = []
    for y in range(height):
        for x in range(0, width, 8):
            byte = 0
            for b in range(8):
                if x + b < width:
                    pixel = img.getpixel((x + b, y))
                    if pixel == 0:
                        byte |= (1 << (7 - b))  # MSB first
            data.append(byte)
    return data

# byte_array = convert_to_gxepd_array(img)
byte_array = convert_to_gfx_bitmap(img)  # Use this for GFX library compatibility

# Print as a C PROGMEM array
print("const unsigned char image[] PROGMEM = {")
for i, byte in enumerate(byte_array):
    if i % 12 == 0:
        print()
    print(f"0x{byte:02X}, ", end="")
print("\n};")

# put to .h file
with open("image.h", "w") as f:
    f.write("const unsigned char image[] PROGMEM = {\n")
    for i, byte in enumerate(byte_array):
        if i % 12 == 0 and i != 0:
            f.write("\n")
        f.write(f"0x{byte:02X}, ")
    f.write("\n};\n")
