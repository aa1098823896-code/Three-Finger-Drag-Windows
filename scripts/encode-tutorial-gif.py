"""Encode rendered tutorial frames with one shared palette and cropped deltas.

Development dependencies: Pillow and NumPy. Run after scripts/export-tutorial-gif.cjs.
Keeps only a few frames in memory; exported teaching media do not run in the app.
"""
import argparse
import json
import re
from pathlib import Path
from PIL import Image, ImageChops, GifImagePlugin
import numpy as np

ROOT = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--frames-dir', type=Path, default=ROOT / 'build' / 'tutorial-frames')
parser.add_argument('--output', type=Path, default=ROOT / 'assets' / 'tutorial.gif')
parser.add_argument('--report', type=Path, default=ROOT / 'build' / 'tutorial-check' / 'gif-validation.json')
args = parser.parse_args()
FRAMES = args.frames_dir
data = json.loads((FRAMES / "frames.json").read_text("utf-8"))
frames = data["frames"]

# Include all six scenes and each gesture phase in the shared palette.
palette_frames = []
scenes = list(dict.fromkeys(frame["scene"] for frame in frames))
with Image.open(FRAMES / frames[0]['file']) as first:
    sample_height = max(1, round(first.height * 190 / first.width))
for scene in scenes:
    for moment in (0.4, 1.5, 3.7, 5.5, 6.7):
        frame = min((f for f in frames if f["scene"] == scene), key=lambda f: abs(f["time"] - moment))
        with Image.open(FRAMES / frame["file"]) as im:
            palette_frames.append(im.convert("RGB").resize((190, sample_height), Image.Resampling.LANCZOS))
sheet = Image.new("RGB", (190 * len(scenes), sample_height * 5), "white")
for i, im in enumerate(palette_frames):
    sheet.paste(im, ((i // 5) * 190, (i % 5) * sample_height))
source_colors = re.findall(r"#([0-9a-fA-F]{6}|[0-9a-fA-F]{3})(?![0-9a-fA-F])", (ROOT / "docs" / "tutorial.html").read_text("utf-8"))
essential = sorted({tuple(bytes.fromhex(value if len(value) == 6 else "".join(c * 2 for c in value))) for value in source_colors})
assert len(essential) < 160, "Too many fixed colors for a GIF palette"
free_colors = 255 - len(essential)
palette = sheet.quantize(colors=free_colors, method=Image.Quantize.MEDIANCUT)
colors = palette.getpalette()[:free_colors * 3] + [channel for rgb in essential for channel in rgb]
colors += colors[:3]
palette.putpalette(colors)
palette_rgb = np.asarray(colors[:255 * 3], dtype=np.int32).reshape(255, 3)
color_lookup = np.full(1 << 24, 255, dtype=np.uint8)


def map_colors(rgb):
    # Use full RGB precision. A coarse color cache can turn nearby whites,
    # fine text edges and small colored controls into the wrong palette entry.
    pixels = np.asarray(rgb, dtype=np.uint32)
    keys = (pixels[:, :, 0] << 16) | (pixels[:, :, 1] << 8) | pixels[:, :, 2]
    missing = np.unique(keys[color_lookup[keys] == 255])
    for start in range(0, missing.size, 2048):
        chunk = missing[start:start + 2048]
        channels = np.stack(((chunk >> 16) & 255, (chunk >> 8) & 255, chunk & 255), axis=1).astype(np.int32)
        distance = ((channels[:, None, :] - palette_rgb[None, :, :]) ** 2).sum(axis=2)
        color_lookup[chunk] = distance.argmin(axis=1).astype(np.uint8)
    indexed = Image.frombytes("P", rgb.size, color_lookup[keys].tobytes())
    indexed.putpalette(colors)
    return indexed


output = args.output
output.parent.mkdir(parents=True, exist_ok=True)
previous = None
duration_carry = 0
with output.open("wb") as stream:
    for frame in frames:
        with Image.open(FRAMES / frame["file"]) as source:
            rgb = source.convert("RGB")
            indexed = map_colors(rgb)
        duration_carry += frame["durationMs"]
        duration = (duration_carry // 10) * 10
        duration_carry -= duration
        if duration == 0:
            continue
        if previous is None:
            for block in GifImagePlugin._get_global_header(indexed, {"loop": 0, "transparency": 255}):
                stream.write(block)
            box = (0, 0, indexed.width, indexed.height)
            delta = indexed
        else:
            current_indices = Image.frombytes("L", indexed.size, indexed.tobytes())
            previous_indices = Image.frombytes("L", previous.size, previous.tobytes())
            diff = ImageChops.difference(current_indices, previous_indices)
            box = diff.getbbox() or (0, 0, 1, 1)
            unchanged = diff.point(lambda value: 0 if value else 255)
            delta = indexed.copy()
            delta.paste(255, (0, 0), unchanged)
        GifImagePlugin._write_frame_data(stream, delta.crop(box), (box[0], box[1]), {"duration": duration, "disposal": 1, "transparency": 255})
        previous = indexed
    stream.write(b";")
with Image.open(output) as gif:
    lengths = []
    for i in range(gif.n_frames):
        gif.seek(i)
        lengths.append(gif.info.get("duration", 0))
    report = {"bytes": output.stat().st_size, "size": gif.size, "frames": gif.n_frames,
              "loop": gif.info.get("loop"), "durationMs": sum(lengths), "text": data["text"]}
    report.update({'fps':data.get('fps'), 'speed':data.get('speed'), 'minimumDelayMs':min(lengths), 'colors':255})
    expected_duration = sum(frame['durationMs'] for frame in frames)
    assert report["loop"] == 0 and report["durationMs"] == expected_duration, report
    assert report['minimumDelayMs'] >= 20, report
args.report.parent.mkdir(parents=True, exist_ok=True)
args.report.write_text(json.dumps(report, indent=2), "utf-8")
print(json.dumps(report))
