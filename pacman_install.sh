#!/bin/bash
# pacman_install.sh

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
TEMP_STAGING="$SCRIPT_DIR/.build_tmp"
OUTPUT_PKG="$SCRIPT_DIR/pacman_setup.run"

echo "--- Generating Self-Extracting Distro-Agnostic Installer ---"

rm -rf "$TEMP_STAGING"
mkdir -p "$TEMP_STAGING"

# 1. Stage Assets, Binary, and Libraries
cp -r "$SCRIPT_DIR/source/pacman-art" "$TEMP_STAGING/"
cp "$SCRIPT_DIR/source/pacman_release" "$TEMP_STAGING/"

# 2. Create the internal installation logic
cat <<EOF > "$TEMP_STAGING/run_install.sh"
#!/bin/bash
echo "Resolving native hardware dependencies..."
if command -v pacman >/dev/null 2>&1; then
    sudo pacman -S --needed --noconfirm sdl2 sdl2_image sdl2_mixer sdl2_ttf
elif command -v apt-get >/dev/null 2>&1; then
    sudo apt-get update
    sudo apt-get install -y libsdl2-2.0-0 libsdl2-image-2.0-0 libsdl2-mixer-2.0-0 libsdl2-ttf-2.0-0
elif command -v dnf >/dev/null 2>&1; then
    sudo dnf install -y SDL2 SDL2_image SDL2_mixer SDL2_ttf
elif command -v zypper >/dev/null 2>&1; then
    sudo zypper install -y libSDL2-2_0-0 libSDL2_image-2_0-0 libSDL2_mixer-2_0-0 libSDL2_ttf-2_0-0
else
    echo "WARNING: Package manager not recognized. Please install SDL2 runtime libraries manually."
fi

TARGET="\$HOME/Games/Pacman"
echo "Installing to \$TARGET..."
mkdir -p "\$TARGET"
cp -r * "\$TARGET/"
rm "\$TARGET/run_install.sh"

echo "Creating Desktop Entry..."
mkdir -p "\$HOME/.local/share/applications"
cat <<DESK > "\$HOME/.local/share/applications/pacman.desktop"
[Desktop Entry]
Name=Pac-Man
Exec=\$TARGET/pacman_release
Icon=\$TARGET/pacman-art/other/icon.png
Type=Application
Categories=Game;
Terminal=false
DESK

if command -v kdialog >/dev/null 2>&1; then
    kdialog --msgbox "Pac-Man installed! You can now find it in your App Menu."
else
    echo "Done! Run \$TARGET/pacman_release to play."
fi
EOF

chmod +x "$TEMP_STAGING/run_install.sh"

# 4. Bake and Wrap in tar.gz for easy sharing
makeself "$TEMP_STAGING" "$OUTPUT_PKG" "Pac-Man" ./run_install.sh
tar -czf pacman_installer.tar.gz pacman_setup.run
rm -rf "$TEMP_STAGING"

echo "Installer ready: pacman_installer.tar.gz"
