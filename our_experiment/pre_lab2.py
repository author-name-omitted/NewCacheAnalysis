#!/usr/bin/env python3
# flake8:noqa
import os
import shutil
import itertools
import argparse
from pathlib import Path
import json

def collect_c_h_files(src_folder):
    """ .c  .h """
    c_and_h_files = []
    for root, _, files in os.walk(src_folder):
        for file in files:
            if file.endswith('.c') or file.endswith('.h'):
                full_path = os.path.join(root, file)
                c_and_h_files.append(full_path)
    return c_and_h_files

def copy_files_to_dest(files, dest_folder):
    """（，）"""
    os.makedirs(dest_folder, exist_ok=True)
    for file_path in files:
        shutil.copy(file_path, dest_folder)

def merge_csv(file1, file2, output_file):
    with open(file1, 'r') as f1, open(file2, 'r') as f2:
        lines1 = f1.readlines()
        lines2 = f2.readlines()

    # ， file1 
    header = lines1[0].strip()
    body1 = lines1[1:]
    body2 = lines2[1:]

    with open(output_file, 'w') as out:
        out.write(header + '\n')
        out.writelines(body1)
        out.writelines(body2)

def read_selected_bench(file_path):
    strings = set()
    try:
        with open(file_path, 'r', encoding='utf-8') as file:
            for line in file:
                # ， set
                strings.add(line.strip())
    except FileNotFoundError:
        print(f"benchmarkselect '{file_path}' ！")
    return strings

def main(input_dir, output_dir, sb_dir):
    bash_dir = str(Path(os.path.abspath(input_dir)))
    if not os.path.isdir(input_dir):
        raise ValueError(f" '{input_dir}' ")

    os.makedirs(output_dir, exist_ok=True)

    subdirs = [d for d in os.listdir(input_dir) 
               if os.path.isdir(os.path.join(input_dir, d))]
    subdirs_sorted = sorted(subdirs)

    counter = 0
    # 
    selected_bench = read_selected_bench(str(Path(os.path.abspath(sb_dir))))
    # 0507
    # local_bench = ["binarysearch", "fir2dim", "huff_dec", "ndes"]
    # remote_bench = ["adpcm_dec", "binarysearch", "expint", "fir", "jfdctint", "statemate"]
    # 0514
    local_bench = ["ndes"]
    remote_bench = ["bitonic", "cubic", "fac", "quicksort", "bitcount",  "bsort", "complex_updates", "cosf", "countnegative", "deg2rad", "filterbank",  "iir", "insertsort", "isqrt", "ludcmp", "matrix1", "rad2deg"]
    
    # for a, b in itertools.permutations(subdirs, 2): # a_b  b_a 
    # for a, b in itertools.combinations(subdirs_sorted, 2): 
    for a in local_bench:
        for b in remote_bench:
            # 
            if len(selected_bench) != 0 and ((a not in selected_bench) or (b not in selected_bench)):
                continue
            combo_name = f"{a}_{b}"
            combo_output_path = os.path.join(output_dir, combo_name)
            os.makedirs(combo_output_path, exist_ok=True)

            # 
            a_files = collect_c_h_files(os.path.join(input_dir, a))
            b_files = collect_c_h_files(os.path.join(input_dir, b))
            copy_files_to_dest(a_files, combo_output_path)
            copy_files_to_dest(b_files, combo_output_path)

            # LoopAnnotations.csv()
            merge_csv(bash_dir + f"/{a}/LoopAnnotations.csv", 
                    bash_dir + f"/{b}/LoopAnnotations.csv",
                        output_dir + f"/{combo_name}/LoopAnnotations.csv")
            
            # CoreInfo.json(`_main`)
            functions = [a+"_main", b+"_main"]
            data = [
                {
                    "core": i,
                    "tasks": [
                        {"function": fname}
                    ]
                }
                for i, fname in enumerate(functions)
            ]
            with open(output_dir + f"/{combo_name}/CoreInfo.json", "w") as f:
                json.dump(data, f, indent=4)
            counter += 1

    # 
    # for a in subdirs_sorted:
    #     # 
    #     if len(selected_bench) != 0 and (a not in selected_bench):
    #         continue
    #     combo_name = f"{a}_{a}"
    #     combo_output_path = os.path.join(output_dir, combo_name)
    #     os.makedirs(combo_output_path, exist_ok=True)

    #     a_files = collect_c_h_files(os.path.join(input_dir, a))
    #     copy_files_to_dest(a_files, combo_output_path)
        
    #     shutil.copy(bash_dir + f"/{a}/LoopAnnotations.csv",
    #                     output_dir + f"/{combo_name}/LoopAnnotations.csv")
        
    #     # CoreInfo.json(`_main`)
    #     functions = [a+"_main", a+"_main"]
    #     data = [
    #         {
    #             "core": i,
    #             "tasks": [
    #                 {"function": fname}
    #             ]
    #         }
    #         for i, fname in enumerate(functions)
    #     ]
    #     with open(output_dir + f"/{combo_name}/CoreInfo.json", "w") as f:
    #         json.dump(data, f, indent=4)
    #     counter += 1

    print(f" {counter} ：{output_dir}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=" .c/.h ")
    parser.add_argument('-s', '--src', type=str, required=True, help='The source file directory, e.g. ./path/to/test')
    parser.add_argument('-t', '--out', type=str, required=True, help='The destination file directory, e.g. ./path/to/test')
    # parser.add_argument('-b', '--bench', type=str, required=True, help='The selected bench file directory, e.g. ./path/to/test')
    parser.add_argument(
        '-b', 
        '--bench', 
        type=str, 
        default="",  # 
        help='The bench file directory (default: ./default/path/to/test)'
    )
    args = parser.parse_args()

    main(args.src, args.out, args.bench)
