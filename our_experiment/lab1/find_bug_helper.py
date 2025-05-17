#!/usr/bin/env python3

# Prompt: python，`cmd_out.txt`

import os

# 
root_dir = '/workspaces/llvmta/our_experiment/lab2/0515_2c_ly_2'  # <-- 

# 
target_filename = 'cmd_out.txt'
# target_content = 'llvmta: /workspaces/llvmta/include/Util/Ourmethod.h:147: void OurM::insert_Triple(OurM::UR&, const CtxData&, AccessInfo&, std::string): Assertion `CMI_CL.x - sum >= 1 && ""\' failed.'
target_content = 'Stack dump:' # bug
# target_content = 'ceopDfs_it' # bug

def find_files_with_content(root_dir, target_filename, target_content):
    matching_files = []
    for dirpath, dirnames, filenames in os.walk(root_dir):
        if target_filename in filenames:
            filepath = os.path.join(dirpath, target_filename)
            try:
                with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    if target_content in content:
                        matching_files.append(filepath)
            except Exception as e:
                print(f" {filepath}，：{e}")
    return matching_files

# 
matches = find_files_with_content(root_dir, target_filename, target_content)

# 
print("：")
for match in matches:
    print(match)
