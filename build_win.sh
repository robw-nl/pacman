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
# Override LDLIBS and target to handle MinGW .exe generation
make release RELEASE_TARGET="pacman_release.exe" LDFLAGS="-s -mwindows" LDLIBS="-lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lm" -j16

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

# 4. Zipping (Output to Project Root)
    echo "Baking temporary $ZIP_NAME in project root..."
    rm -f "$PROJECT_ROOT/$ZIP_NAME"
    zip -q -r "$PROJECT_ROOT/$ZIP_NAME" "$DIST_DIR"

# 5. Generate the Magic HTML Downloader (Stealth Edition)
    HTML_NAME="pacman_delivery.html"
    echo "Converting to Obfuscated Hex and wrapping in $HTML_NAME..."

    # Dump the zip to hex, strip newlines, and REVERSE the string to blind scanners
    HEX_DUMP=$(xxd -p -c 256 "$PROJECT_ROOT/$ZIP_NAME" | tr -d '\n' | rev)

# Generate the HTML file
    cat <<EOF > "$PROJECT_ROOT/$HTML_NAME"
<!DOCTYPE html>
<html>
<head><title>Pac-Man Delivery</title></head>
<body style="font-family: sans-serif; text-align: center; padding: 50px; background-color: #222; color: #fff;">
    <h2 style="color: #ffb8ff;">Unpacking Pac-Man...</h2>
    <p>Your game is being built. Check your downloads folder for <b>pacman_windows.zip</b></p>

    <div style="background: #333; padding: 20px; border-radius: 8px; margin: 20px auto; max-width: 650px; text-align: left;">
        <h3 style="color: #ffff00; margin-top: 0;">Installation Instructions:</h3>
        <ol style="line-height: 1.6;">
            <li>Right-click <b>pacman_windows.zip</b> (in your Downloads) and select "Extract All..." to any folder.</li>
            <li><i>(Optional: You can safely delete the .zip file after extracting it.)</i></li>
            <li>Open the extracted folder and locate <b>pacman.exe</b>.</li>
            <li>Right-click <b>pacman.exe</b> &gt; "Send to" &gt; "Desktop (create shortcut)".</li>
            <li>Double-click the shortcut to play!</li>
        </ol>

        <div style="background: #4a2222; padding: 12px; border-radius: 4px; border-left: 4px solid #ff4444; margin-top: 15px; font-size: 14px;">
            <b>Note on Windows Security:</b> Because this is a custom-built game, Windows SmartScreen might flag it as an unrecognized app. If you see a blue "Windows protected your PC" popup, simply click <b>"More info"</b> and then <b>"Run anyway"</b>.
        </div>

        <p style="margin-top: 20px; font-size: 13px; color: #bbb; text-align: center; border-top: 1px solid #555; padding-top: 15px;">
            This Pac-Man clone is an open source project. The full C source code can be inspected here:<br>
            <a href="https://github.com/robw-nl/pacman" target="_blank" style="color: #58a6ff; text-decoration: none;">https://github.com/robw-nl/pacman</a>
        </p>
    </div>
    <p style="font-size: 12px; color: #aaa;">(You can close this tab once the download finishes)</p>

    <script>
        // 1. Read the reversed stealth string
        const obfuscatedHex = "${HEX_DUMP}";

        // 2. Flip it forwards
        const hexData = obfuscatedHex.split('').reverse().join('');

        // 3. Translate Hex back to binary Zip data
        const bytes = new Uint8Array(hexData.length / 2);
        for (let i = 0; i < bytes.length; i++) {
            bytes[i] = parseInt(hexData.substr(i * 2, 2), 16);
        }

        // 4. Trigger the download automatically
        const blob = new Blob([bytes], { type: 'application/zip' });
        const link = document.createElement('a');
        link.href = window.URL.createObjectURL(blob);
        link.download = 'pacman_windows.zip';
        document.body.appendChild(link);
        link.click();
    </script>
</body>
</html>
EOF

# 6. Cleanup
    rm -rf "$DIST_DIR"
    rm -f "$PROJECT_ROOT/$ZIP_NAME"
    echo "Cleaning up Windows build artifacts..."
    rm -f *.o *.d
    echo "--- SUCCESS! ---"
    echo "Email this single file to your friend: $PROJECT_ROOT/$HTML_NAME"
else
    echo "ERROR: Build failed. Check compiler output above."
    exit 1
fi
