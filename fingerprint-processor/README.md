# Fingerprint Processor - Linux Version

Fast, consistent fingerprint processing for database insertion. Built for GitHub Codespaces development and Linux deployment.

## Quick Start

### 1. Open in GitHub Codespaces
- Push this code to a GitHub repository
- Click "Code" → "Codespaces" → "Create codespace"
- Everything will be automatically installed!

### 2. Build the Project
```bash
# Make build script executable
chmod +x scripts/build.sh

# Build the project
./scripts/build.sh
```

### 3. Test with Sample Data
```bash
# Create test data directory
mkdir -p test_data

# Copy some fingerprint images to test_data/
# (BMP, JPG, PNG, TIFF formats supported)

# Run the processor
./build/bin/fingerprint_processor -i test_data -v
```

## Command Line Options

```bash
./build/bin/fingerprint_processor [options]

Options:
  -i <dir>     Input directory (default: test_data)
  -o <dir>     Output directory (default: output)  
  -n <count>   Max files to process (default: all)
  -v           Verbose output
  -h           Show help
```

## Examples

```bash
# Process all images in test_data/ with verbose output
./build/bin/fingerprint_processor -i test_data -v

# Process only first 10 images
./build/bin/fingerprint_processor -i test_data -n 10

# Process images from custom directory
./build/bin/fingerprint_processor -i /path/to/fingerprints -o /path/to/output
```

## Current Status

✅ **Phase 1: Core Pipeline Foundation**
- Logger system with Linux compatibility
- High-precision timing
- File management with Linux file I/O
- Core point detection with SIMD optimization
- ROI extraction (101x101 pixels exactly)

🔄 **Next: Phase 2 - Feature Extraction**
- Ridge density calculation
- Minutiae density analysis  
- Pattern type classification
- Zernike moment computation

## Project Structure

```
fingerprint-processor/
├── .devcontainer/          # GitHub Codespaces config
├── src/                    # Source code
│   ├── main.cpp           # Entry point
│   ├── core/              # Core processing
│   ├── utils/             # Utilities
│   └── database/          # Database integration
├── scripts/               # Build scripts
├── test_data/             # Sample images
└── build/                 # Build output
```

## Performance Targets

- **Per Image**: ~3.2ms total processing time
- **Throughput**: ~310 images/second (on development hardware)
- **Consistency**: Identical ROI extraction every time
- **Quality**: Biological addresses matching Python implementation

## Development Notes

- Built with GCC on Ubuntu 22.04
- Uses OpenCV for image processing
- SQLite for database operations
- AVX2 SIMD instructions when available
- Thread-safe design for batch processing
