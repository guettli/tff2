#!/usr/bin/env python3
"""Generate the tray/program icons (src/platform/windows/tff_*.ico).

A rounded square with a white "T" (blue = active, grey = paused), at 16 to 64
px, written as 32-bit BMP icons. Run: python3 scripts/windows/make_icons.py
"""
import struct
from pathlib import Path

OUT = Path(__file__).resolve().parents[2] / "src" / "platform" / "windows"


def render(size, fill):
    """Return BGRA rows (top-down) for one icon size."""
    radius = size * 0.22
    rows = []
    for y in range(size):
        row = bytearray()
        for x in range(size):
            # rounded-square coverage with 4x4 supersampling
            cov = 0
            letter = 0
            for sy in range(4):
                for sx in range(4):
                    px = x + (sx + 0.5) / 4
                    py = y + (sy + 0.5) / 4
                    cx = min(max(px, radius), size - radius)
                    cy = min(max(py, radius), size - radius)
                    if (px - cx) ** 2 + (py - cy) ** 2 <= radius**2:
                        cov += 1
                        # the "T": top bar and stem
                        u, v = px / size, py / size
                        if (0.2 <= u <= 0.8 and 0.2 <= v <= 0.36) or (
                            0.42 <= u <= 0.58 and 0.2 <= v <= 0.8
                        ):
                            letter += 1
            a = cov / 16
            t = letter / max(cov, 1)
            r = int(fill[0] * (1 - t) + 255 * t)
            g = int(fill[1] * (1 - t) + 255 * t)
            b = int(fill[2] * (1 - t) + 255 * t)
            row += bytes((b, g, r, int(a * 255)))
        rows.append(bytes(row))
    return rows


def bmp_image(size, fill):
    rows = render(size, fill)
    header = struct.pack("<IiiHHIIiiII", 40, size, size * 2, 1, 32, 0, 0, 0, 0, 0, 0)
    pixels = b"".join(reversed(rows))  # bottom-up
    mask_row = ((size + 31) // 32) * 4
    mask = bytes(mask_row * size)  # alpha channel decides; AND mask all zero
    return header + pixels + mask


def write_ico(path, fill, sizes=(16, 20, 24, 32, 40, 48, 64)):
    images = [bmp_image(s, fill) for s in sizes]
    out = struct.pack("<HHH", 0, 1, len(images))
    offset = 6 + 16 * len(images)
    for s, img in zip(sizes, images):
        dim = 0 if s == 256 else s
        out += struct.pack("<BBBBHHII", dim, dim, 0, 0, 1, 32, len(img), offset)
        offset += len(img)
    path.write_bytes(out + b"".join(images))


if __name__ == "__main__":
    write_ico(OUT / "tff_active.ico", (37, 99, 235))
    write_ico(OUT / "tff_paused.ico", (120, 120, 128))
    print("wrote", OUT / "tff_active.ico", OUT / "tff_paused.ico")
