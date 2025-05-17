#!/usr/bin/env python3

# Prompt: python，，/build，

import os
import shutil

def copy_dirs_without_build(src_root, dst_root):
    # src_root
    for dirpath, dirnames, filenames in os.walk(src_root):
        # ，
        if dirpath == src_root:
            for subdir in dirnames:
                subdir_path = os.path.join(src_root, subdir)
                build_path = os.path.join(subdir_path, 'build')
                
                #  build 
                if not os.path.exists(build_path):
                    dst_path = os.path.join(dst_root, subdir)
                    print(f"Copying '{subdir_path}' to '{dst_path}' because 'build' folder not found.")
                    
                    # ，
                    if os.path.exists(dst_path):
                        shutil.rmtree(dst_path)
                    
                    shutil.copytree(subdir_path, dst_path)
                else:
                    print(f"Skipping '{subdir_path}' because 'build' folder exists.")

# =======  =======
if __name__ == "__main__":
    source_folder = "./"  # 
    destination_folder = "/workspaces/llvmta/our_experiment/lab1/0503_2c_zw2" 

    # （）
    if not os.path.exists(destination_folder):
        os.makedirs(destination_folder)

    copy_dirs_without_build(source_folder, destination_folder)
