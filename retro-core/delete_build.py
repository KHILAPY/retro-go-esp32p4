import shutil
import os

build_dir = r'c:\Users\A\Desktop\retro-go-1.4x-gba - qoder\retro-core\build'
if os.path.exists(build_dir):
    shutil.rmtree(build_dir)
    print(f"Deleted: {build_dir}")
else:
    print(f"Directory not found: {build_dir}")