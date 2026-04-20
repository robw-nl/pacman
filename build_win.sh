#!/bin/bash
# build_win.sh - Location: Project Root

# 1. Setup Environment
export CC=x86_64-w64-mingw32-gcc
export PKG_CONFIG=x86_64-w64-mingw32-pkg-config
PROJECT_ROOT=$(pwd)
SOURCE_DIR="$PROJECT_ROOT/source"
ZIP_NAME="pacman_windows_dist.zip"
DIST_DIR="./pacman"

echo "--- Entering Source & Generating Makefile ---"
cd "$SOURCE_DIR" || exit 1
./makefile_gen.sh

echo "--- Compiling for Windows (GUI Mode) ---"
make clean
# Override LDLIBS to force exact MinGW linking order
make release LDFLAGS="-s -mwindows" LDLIBS="-lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lm" -j16

# 2. Package and Collect DLLs
if [ -f "pacman_release.exe" ]; then
    echo "Found binary. Preparing distribution folder..."
    rm -rf "$DIST_DIR"
    mkdir -p "$DIST_DIR/pacman-art"

    mv "pacman_release.exe" "$DIST_DIR/pacman.exe"
    cp -r ./pacman-art/* "$DIST_DIR/pacman-art/"

    # 3. Gather ALL Windows DLLs (Recursive Autodiscovery)
    DLL_PATH="/usr/x86_64-w64-mingw32/bin"
    echo "Dynamically resolving MinGW DLL dependencies..."

    resolve_dlls() {
        local target="$1"
        local deps=$(x86_64-w64-mingw32-objdump -p "$target" 2>/dev/null | grep -i "DLL Name" | awk '{print $3}')
        for dll in $deps; do
            if [ ! -f "$DIST_DIR/$dll" ]; then
                local full_path=$(find "$DLL_PATH" -maxdepth 1 -iname "$dll" -print -quit)
                if [ -n "$full_path" ]; then
                    cp "$full_path" "$DIST_DIR/"
                    resolve_dlls "$full_path"
                fi
            fi
        done
}
    resolve_dlls "$DIST_DIR/pacman.exe"

    # 4. Create Readme
    echo "Writing readme.txt..."
    cat <<EOF > "$DIST_DIR/readme.txt"
Pac-Man (Windows Release)
=========================
1. Extract this entire 'pacman' folder to any location on your PC.
2. Double-click 'pacman.exe' to play.
EOF

# 5. Zipping (Output to Project Root)
    echo "Baking $ZIP_NAME in project root..."
    rm -f "$PROJECT_ROOT/$ZIP_NAME"
    zip -q -r "$PROJECT_ROOT/$ZIP_NAME" "$DIST_DIR"

# 6. Cleanup
    rm -rf "$DIST_DIR"
    echo "Cleaning up Windows build artifacts..."
    rm -f *.o *.d
    echo "--- SUCCESS! ---"
    echo "Final Package: $PROJECT_ROOT/$ZIP_NAME"
else
    echo "ERROR: Build failed. Check compiler output above."
    exit 1
fi
