#!/bin/bash

# Build D2 diagrams to SVG with sketch mode
# This script finds all .d2 files and converts them to SVG with sketch styling

set -e  # Exit on any error

echo "🔧 Building D2 diagrams to SVG with sketch mode..."

# Check if d2 is installed
if ! command -v d2 &> /dev/null; then
    echo "❌ Error: d2 is not installed"
    echo "Please install d2 from https://d2lang.com/tour/install"
    exit 1
fi

# Find all .d2 files in the current directory and subdirectories
d2_files=$(find . -name "*.d2" -type f)

if [ -z "$d2_files" ]; then
    echo "⚠️  No .d2 files found in the current directory or subdirectories"
    exit 0
fi

# Counter for processed files
processed=0
errors=0

# Process each .d2 file
for d2_file in $d2_files; do
    # Get the base name without extension
    base_name=$(basename "$d2_file" .d2)
    dir_name=$(dirname "$d2_file")
    svg_file="$dir_name/$base_name.svg"
    
    echo "📝 Processing: $d2_file"
    
    # Build SVG with sketch mode
    if d2 --sketch "$d2_file" "$svg_file"; then
        echo "✅ Created: $svg_file"
        ((processed++))
    else
        echo "❌ Error building: $d2_file"
        ((errors++))
    fi
done

echo ""
echo "📊 Build Summary:"
echo "   Processed: $processed files"
echo "   Errors: $errors files"

if [ $errors -eq 0 ]; then
    echo "🎉 All diagrams built successfully!"
else
    echo "⚠️  Some diagrams failed to build. Check the errors above."
    exit 1
fi 