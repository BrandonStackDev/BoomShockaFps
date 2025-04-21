#!/bin/bash

set -e

APP_NAME="BoomShocka"
EXE_NAME="game"
SRC_FILES="*.c"
ASSETS_DIRS=("textures" "models" "maps")

# Output paths
APP_BUNDLE="$APP_NAME.app"
MACOS_DIR="$APP_BUNDLE/Contents/MacOS"
RES_DIR="$APP_BUNDLE/Contents/Resources"
PLIST="$APP_BUNDLE/Contents/Info.plist"

# Flags
CFLAGS="-I/usr/local/include -DPLATFORM_DESKTOP -DMEMORY_SAFE_MODE"
LDFLAGS="/usr/local/lib/libraylib.a -lm \
  -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo"

echo "🛠️  Building Mac universal binary..."

# Build x86_64 and arm64 versions
clang -arch x86_64 $CFLAGS $SRC_FILES -o ${EXE_NAME}_x86_64 $LDFLAGS
clang -arch arm64 $CFLAGS $SRC_FILES -o ${EXE_NAME}_arm64 $LDFLAGS

# Combine into universal binary
lipo -create -output $EXE_NAME ${EXE_NAME}_x86_64 ${EXE_NAME}_arm64
rm ${EXE_NAME}_x86_64 ${EXE_NAME}_arm64

echo "📦 Packaging .app bundle..."

# Create app structure
mkdir -p "$MACOS_DIR"
mkdir -p "$RES_DIR"

# Move binary
cp "$EXE_NAME" "$MACOS_DIR/"

# Copy assets
for dir in "${ASSETS_DIRS[@]}"; do
    if [ -d "$dir" ]; then
        cp -R "$dir" "$RES_DIR/"
    fi
done

# Create Info.plist
cat > "$PLIST" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
 "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>$APP_NAME</string>
    <key>CFBundleExecutable</key>
    <string>$EXE_NAME</string>
    <key>CFBundleIdentifier</key>
    <string>com.example.$APP_NAME</string>
    <key>CFBundleVersion</key>
    <string>1.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
</dict>
</plist>
EOF

zip -r BoomShocka.zip BoomShocka.app

echo "✅ Done! Run with:"
echo "  open \"$APP_BUNDLE\""
