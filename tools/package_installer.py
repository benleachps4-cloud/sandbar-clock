"""Copy the compiled firmware into docs/ for the browser installer.

Build both boards first, from the firmware folder:
    pio run -e esp32c3 -e esp32s3
Then run this script from anywhere. It doesn't need any extra packages.
"""
import pathlib
import shutil

ROOT = pathlib.Path(__file__).resolve().parent.parent
BOOT_APP0 = pathlib.Path.home() / ".platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"

for board in ("esp32c3", "esp32s3"):
    build = ROOT / "firmware/.pio/build" / board
    target = ROOT / "docs/firmware" / board
    target.mkdir(parents=True, exist_ok=True)
    for source in (build / "bootloader.bin", build / "partitions.bin", build / "firmware.bin", BOOT_APP0):
        shutil.copy2(source, target / source.name)
        print(f"{board}/{source.name}: {source.stat().st_size} bytes")
