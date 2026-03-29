# Usage: "python convert.py image_name.png [-a]"
# -a: include alpha data
# outputs: image_name.h
# Use "pip install pillow" if you don't have PIL installed

import os
import sys
from PIL import Image

input_file = sys.argv[1]  # input

output_name = input_file.split(".")[0]     # output

def convert_to_h_file(input_path, output_name, use_alpha = False):
    if not os.path.exists(input_path):
        print(f"cannot find '{input_path}', wrong file name")
        return

    img = Image.open(input_path).convert('RGBA')
    pixels = list(img.get_flattened_data())  # [(r,g,b,a), ...]

    with open(f"{output_name}.h", "w") as f:
        f.write(f"#define {output_name.upper()}_WIDTH {img.width}\n")
        f.write(f"#define {output_name.upper()}_HEIGHT {img.height}\n\n")

        # --- RGB565 DATA ---
        f.write(f"const uint16_t {output_name}_data[] = {{\n    ")

        for i, (r, g, b, a) in enumerate(pixels):
            rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

            end_char = ",\n    " if (i + 1) % 12 == 0 else ", "
            f.write(f"0x{rgb565:04X}{end_char}")

        f.write("\n};\n\n")


        # --- 1-BIT ALPHA MASK ---
        if use_alpha:
            f.write(f"const uint8_t {output_name}_alpha_mask[] = {{\n    ")

            byte = 0
            bit_count = 0
            byte_index = 0

            for i, (r, g, b, a) in enumerate(pixels):
                # Threshold: adjust if needed (0–255)
                alpha_bit = 1 if a >= 128 else 0

                # Pack bits MSB first
                byte = (byte << 1) | alpha_bit
                bit_count += 1

                if bit_count == 8:
                    end_char = ",\n    " if (byte_index + 1) % 12 == 0 else ", "
                    f.write(f"0x{byte:02X}{end_char}")
                    byte = 0
                    bit_count = 0
                    byte_index += 1

            # Handle remaining bits (pad with 0s)
            if bit_count > 0:
                byte <<= (8 - bit_count)
                f.write(f"0x{byte:02X}")

            f.write("\n};\n")

convert_to_h_file(input_file, output_name, '-a' in sys.argv)
