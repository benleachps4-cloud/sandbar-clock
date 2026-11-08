# Keeps the builder's home folder, and so their username, out of the compiled firmware.
import os

Import("env")

home = os.path.expanduser("~").replace("\\", "/")
env.Append(CCFLAGS=[f"-ffile-prefix-map={home}=~"])
