#!/usr/bin/env python3

# Prompt: python，/build
import os

def find_immediate_subfolders_without_build(parent_dir):
    """
    （1），/build
    :param parent_dir: 
    :return: 
    """
    valid_subfolders = []

    # （1）
    for subfolder in os.listdir(parent_dir):
        subfolder_path = os.path.join(parent_dir, subfolder)
        
        # /build
        if os.path.isdir(subfolder_path):
            has_build = 'build' in os.listdir(subfolder_path)
            if not has_build:
                valid_subfolders.append(subfolder_path)

    return valid_subfolders

if __name__ == "__main__":
    # 
    search_dir = "/workspaces/llvmta/our_experiment/lab1/0506_2c_our"
    if not os.path.isdir(search_dir):
        print(f":  {search_dir} !")
        exit(1)

    # 
    result = find_immediate_subfolders_without_build(search_dir)
    print("\n（/build）:")
    for folder in result:
        print(folder)