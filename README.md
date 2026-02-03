# LadybugExport

A command-line tool that extends `ladybugProcessStream.exe` with two additional features:

1. **Export of individual camera images** (`-x 6processed`)
2. **Export panorama with rotation angle** (`-q "Front X -Down Y"`)

This tool replaces the manual workflow in LadybugCapPro with automated command-line processing.

## Supported Cameras

| Camera | Status | Data Format |
|--------|--------|-------------|
| Ladybug6 | ✅ Tested | JPEG compressed (format 4) |
| Ladybug5+ | ✅ Tested | 16-bit raw (format 8) |
| Ladybug5 | ✅ Compatible | Various formats |
| Ladybug3 | ✅ Compatible | Various formats |

## Requirements

- **OS**: Windows 10/11 x64
- **GPU**: OpenGL-capable graphics card (required for Ladybug5+/6 panorama rendering)
- **SDK**: Teledyne FLIR Ladybug SDK (64-bit) installed at `C:\Program Files\Teledyne\Ladybug`

---

## Quick Start

### Export 6 Camera Images

```cmd
LadybugExport.exe -i "C:\data\recording.pgr" -o "C:\output\cameras" -c hq -x 6processed
```

### Export Panorama with Rotation

```cmd
LadybugExport.exe -i "C:\data\recording.pgr" -o "C:\output\pano" -w 2048x1024 -c hq -q "Front 5 -Down 0"
```

---

## Complete Command-Line Reference

### Basic Syntax

```cmd
LadybugExport.exe -i <input.pgr> -o <output_prefix> [OPTIONS]
```

### Required Options

| Option | Description | Example |
|--------|-------------|---------|
| `-i <file>` | Input .pgr stream file | `-i recording.pgr` |
| `-o <prefix>` | Output file prefix (path + filename prefix) | `-o C:\output\image` |

### Frame Selection

| Option | Description | Default | Example |
|--------|-------------|---------|---------|
| `-r NNN-NNN` | Frame range to process | All frames | `-r 0-99` |

### Output Configuration

| Option | Description | Default | Example |
|--------|-------------|---------|---------|
| `-w WIDTHxHEIGHT` | Output image size in pixels | `2048x1024` | `-w 4096x2048` |
| `-f <format>` | Output image format | `jpg` | `-f png` |
| `-t <type>` | Render type for panorama | `pano` | `-t dome` |

### Image Format Options (`-f`)

| Value | Description | File Size | Quality |
|-------|-------------|-----------|---------|
| `jpg` | JPEG (lossy compression) | Small | Good (default) |
| `png` | PNG (lossless compression) | Medium | Excellent |
| `bmp` | Windows Bitmap (uncompressed) | Large | Excellent |
| `tiff` | TIFF (lossless) | Large | Excellent |

### Render Type Options (`-t`)

| Value | Description |
|-------|-------------|
| `pano` | Equirectangular panoramic view (default) |
| `dome` | Dome/fisheye view |
| `spherical` | Spherical projection view |
| `rectify-0` | Rectified image from camera 0 |
| `rectify-1` | Rectified image from camera 1 |
| `rectify-2` | Rectified image from camera 2 |
| `rectify-3` | Rectified image from camera 3 |
| `rectify-4` | Rectified image from camera 4 |
| `rectify-5` | Rectified image from camera 5 |

### Color Processing Options (`-c`)

| Value | Description | Speed | Quality |
|-------|-------------|-------|---------|
| `hq` | High quality linear interpolation | Medium | Best (default) |
| `hq-gpu` | High quality using GPU acceleration | Fast | Best |
| `edge` | Edge-sensing interpolation | Medium | Good |
| `near` | Nearest neighbor interpolation | Fast | Fair |
| `down4` | Downsample by 4 | Very Fast | Low |
| `down16` | Downsample by 16 | Fastest | Lowest |
| `mono` | Monochrome output | Fast | N/A |

### Blending Options

| Option | Description | Default | Example |
|--------|-------------|---------|---------|
| `-b <pixels>` | Blending width for panorama stitching | `100` | `-b 150` |

### Extended Options (NEW)

| Option | Description | Example |
|--------|-------------|---------|
| `-x 6processed` | Export all 6 processed camera images | `-x 6processed` |
| `-q "Front X -Down Y"` | Rotation angle for panorama | `-q "Front 5 -Down 0"` |

---

## Rotation Angle Format

The `-q` option uses the **same format as LadybugCapPro GUI**:

```
-q "Front <pitch> -Down <yaw>"
```

### Parameters

| Parameter | Axis | Positive Value | Negative Value |
|-----------|------|----------------|----------------|
| **Front** | Pitch (X-axis) | Look up | Look down |
| **Down** | Yaw (Y-axis) | Rotate right | Rotate left |

### Examples

| Command | Effect |
|---------|--------|
| `-q "Front 5 -Down 0"` | Tilt camera up 5 degrees |
| `-q "Front -10 -Down 0"` | Tilt camera down 10 degrees |
| `-q "Front 0 -Down 45"` | Rotate view right 45 degrees |
| `-q "Front 0 -Down -90"` | Rotate view left 90 degrees |
| `-q "Front 5 -Down -90"` | Tilt up 5°, rotate left 90° |

---

## Output File Naming

### 6 Camera Export (`-x 6processed`)

```
<output_prefix>_<frame>_cam<camera>.jpg
```

Example with `-o C:\output\cameras`:
```
C:\output\cameras_000000_cam0.jpg
C:\output\cameras_000000_cam1.jpg
C:\output\cameras_000000_cam2.jpg
C:\output\cameras_000000_cam3.jpg
C:\output\cameras_000000_cam4.jpg
C:\output\cameras_000000_cam5.jpg
C:\output\cameras_000001_cam0.jpg
...
```

### Panorama Export

```
<output_prefix>_<frame>.jpg
```

Example with `-o C:\output\pano`:
```
C:\output\pano_000000.jpg
C:\output\pano_000001.jpg
...
```

### Camera Numbering

| Camera | Position |
|--------|----------|
| cam0 | Side camera 0 |
| cam1 | Side camera 1 |
| cam2 | Side camera 2 |
| cam3 | Side camera 3 |
| cam4 | Side camera 4 |
| cam5 | Top/bottom camera |

---

## Python Integration Example

```python
import subprocess

stream_file = r"C:\data\recording.pgr"
output_path = r"C:\output\cameras"

# Export 6 processed camera images
ladybug_command = [
    r"C:\Program Files\Teledyne\Ladybug\bin64\LadybugExport.exe",
    "-i", stream_file,
    "-o", output_path,
    "-c", "hq",
    "-x", "6processed"
]
subprocess.run(ladybug_command)

# Export panorama with rotation
ladybug_command = [
    r"C:\Program Files\Teledyne\Ladybug\bin64\LadybugExport.exe",
    "-i", stream_file,
    "-o", output_path,
    "-w", "2048x1024",
    "-c", "hq",
    "-q", "Front 5 -Down 0"
]
subprocess.run(ladybug_command)
```

---

## Installation

### Option 1: Copy to SDK folder (Recommended)

1. Copy `LadybugExport.exe` to `C:\Program Files\Teledyne\Ladybug\bin64\`
2. The SDK DLLs are already in that folder, so no additional setup needed

### Option 2: Standalone folder

1. Create a folder for the tool (e.g., `C:\Tools\LadybugExport\`)
2. Copy `LadybugExport.exe` to the folder
3. Copy all DLLs from `C:\Program Files\Teledyne\Ladybug\bin64\` to the same folder

### Required DLLs

If running from a standalone folder, these DLLs are required:
- `ladybug.dll`
- `ladybuggeom.dll`
- `ladybugstream.dll`
- `FreeImage.dll`
- `cudart64_*.dll` (CUDA runtime)
- Additional supporting DLLs from SDK bin64 folder

---

## Building from Source

1. Install Ladybug SDK from https://www.flir.com/support-center/iis/machine-vision/downloads/ladybug-sdk/
2. Open `LadybugExport.sln` in Visual Studio 2019 or later
3. Select **Release | x64** configuration
4. Build the solution (F7)
5. Copy `bin\Release\LadybugExport.exe` to SDK bin64 folder

---

## Error Reference

### Initialization Errors

| Error Message | Cause | Solution |
|---------------|-------|----------|
| `Error: Input file not specified` | Missing `-i` parameter | Add `-i <file.pgr>` to command |
| `Error [ladybugInitializeStreamForReading]: Unable to open file` | File not found or corrupted | Check file path and ensure .pgr file exists |
| `Error [ladybugLoadConfig]: Failed to load configuration file` | Invalid or missing config in stream | Test file in LadybugCapPro first |
| `Error [ladybugCreateContext]: Failed to create context` | SDK initialization failed | Reinstall Ladybug SDK |

### Processing Errors

| Error Message | Cause | Solution |
|---------------|-------|----------|
| `Warning: Could not convert frame N: An invalid argument was passed` | Data format mismatch | Tool auto-handles this for LB5+/LB6; contact support if persists |
| `Error [ladybugConvertImage]: Failed to convert image` | Image conversion failed | Try different color processing method (`-c near`) |
| `Warning: Could not read frame N` | Corrupted frame in stream | Skip frame with `-r` option to select valid range |

### Panorama Rendering Errors

| Error Message | Cause | Solution |
|---------------|-------|----------|
| `Error [ladybugConfigureOutputImages]: Not enough resource for OpenGL texture` | No GPU or software rendering attempted | Ensure OpenGL-capable GPU is available |
| `Error [ladybugRenderOffScreenImage]: Failed to render panorama` | GPU rendering failed | Update graphics drivers; ensure GPU supports OpenGL 3.0+ |
| `Error [ladybugUpdateTextures]: Failed to update textures` | Texture memory error | Close other GPU-intensive applications |
| `Warning: Could not set rotation params` | Invalid rotation values | Check rotation format: `-q "Front X -Down Y"` |

### File Output Errors

| Error Message | Cause | Solution |
|---------------|-------|----------|
| `Warning: Could not save camera N image` | Write permission or disk full | Check output folder permissions and disk space |
| `Error: Could not save panorama` | Output path invalid | Ensure output folder exists or use valid path |
| `Error: Failed to allocate texture buffer` | Out of memory | Close other applications; reduce resolution (`-w`) |

### DLL/Runtime Errors

| Error Message | Cause | Solution |
|---------------|-------|----------|
| `The code execution cannot proceed because ladybug.dll was not found` | Missing SDK DLLs | Copy exe to SDK bin64 folder OR copy all DLLs to exe folder |
| `The code execution cannot proceed because cudart64_*.dll was not found` | Missing CUDA runtime | Install CUDA runtime or copy DLLs from SDK |
| `Entry point not found` | SDK version mismatch | Ensure SDK version matches the built executable |

### Common Warnings (Can Be Ignored)

| Warning Message | Meaning |
|-----------------|---------|
| `MapSMtoCores for SM 12.0 is undefined. Default to use 128 Cores/SM` | New GPU architecture; SDK uses default settings |
| `Warning: Could not initialize alpha masks` | Alpha mask init failed; blending may be affected |
| `Warning: Could not set blending params` | Blending width setting failed; uses default |

---

## Supported Data Formats

The tool automatically detects and handles these Ladybug data formats:

| Format ID | Name | Description | Camera |
|-----------|------|-------------|--------|
| 1 | RAW8 | 8-bit raw Bayer | LB2/LB3 |
| 2 | JPEG8 | JPEG compressed 8-bit | LB2/LB3 |
| 3 | COLOR_SEP_RAW8 | Color separated 8-bit | LB3/LB5 |
| 4 | COLOR_SEP_JPEG8 | Color separated JPEG 8-bit | LB5/LB6 |
| 5 | HALF_HEIGHT_RAW8 | Half-height 8-bit raw | LB5 |
| 6 | HALF_HEIGHT_JPEG8 | Half-height JPEG 8-bit | LB5 |
| 7 | RAW16 | 16-bit raw | LB5+ |
| 8 | HALF_HEIGHT_RAW16 | Half-height 16-bit raw | LB5+ |
| 9 | COLOR_SEP_JPEG12 | 12-bit JPEG | LB5+/LB6 |
| 10 | HALF_HEIGHT_JPEG12 | Half-height 12-bit JPEG | LB5+/LB6 |

---

## Technical Notes

### GPU Requirements

Panorama rendering on Ladybug3 and later cameras **requires GPU hardware acceleration**. Software rendering is not supported for these camera models.

**Minimum requirements:**
- OpenGL 3.0 compatible graphics card
- 1GB VRAM recommended
- Updated graphics drivers

### Memory Usage

| Operation | Approximate Memory |
|-----------|-------------------|
| 6 camera export (4K) | ~1.5 GB |
| Panorama export (4K) | ~2 GB |
| Panorama export (8K) | ~4 GB |

### Performance Tips

1. Use `-c hq-gpu` for GPU-accelerated color processing
2. Use `-c down4` for faster processing with lower quality
3. Process smaller frame ranges with `-r` for testing
4. Close other GPU applications when processing

---

## License

Uses Teledyne FLIR Ladybug SDK. Subject to SDK license agreement.
