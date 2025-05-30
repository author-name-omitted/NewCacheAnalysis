#!/usr/bin/env python3
# flake8:noqa


import os
import re
import pandas as pd
from glob import glob

def process_directories(root_dir):
    pattern = re.compile(r'0515_2c_([a-z]+)_(\d+)')
    dirs = [d for d in os.listdir(root_dir) 
            if os.path.isdir(os.path.join(root_dir, d)) and pattern.match(d)]
    
    if not dirs:
        print("")
        return
    
    # 
    param_groups = {}
    for d in dirs:
        match = pattern.match(d)
        algo, param = match.groups()
        if param not in param_groups:
            param_groups[param] = {}
        param_groups[param][algo] = os.path.join(root_dir, d)
    
    # 
    summary_path = os.path.join(root_dir, "lab2_summary.csv")
    with open(summary_path, 'w') as f:
        f.write("")  # 
        sorted_params = sorted(param_groups.keys(), key=lambda x: int(x))
    
    for param in sorted_params:
        process_param_group(param, param_groups[param], summary_path)
    # 
    # for param, algos in param_groups.items():
    #     process_param_group(param, algos, summary_path)
    
    print(f"see result at: {summary_path}")

def process_param_group(param, algos, summary_path):
    # 
    with open(summary_path, 'a') as f:
        f.write(f"assoc={param}\n")
    
    # lyintra_wcet.csv
    if 'ly' in algos:
        with open(summary_path, 'a') as f:
            f.write(f"liang_total\n")
        ly_dir = algos['ly']
        intra_path = os.path.join(ly_dir, 'intra_wcet.csv')
        # print(f"{param} {algos}")
        # print(intra_path)
        if os.path.exists(intra_path):
            process_and_append(intra_path, summary_path, 'ly')
    
    # zwwceet.csv
    if 'zw' in algos:
        with open(summary_path, 'a') as f:
            f.write(f"zhang_inter\n")
        zw_dir = algos['zw']
        wceet_path = os.path.join(zw_dir, 'wceet.csv')
        if os.path.exists(wceet_path):
            process_and_append(wceet_path, summary_path, 'zw')
    
    # ourwceet.csv
    if 'our' in algos:
        with open(summary_path, 'a') as f:
            f.write(f"our_inter\n")
        our_dir = algos['our']
        wceet_path = os.path.join(our_dir, 'wceet.csv')
        if os.path.exists(wceet_path):
            process_and_append(wceet_path, summary_path, 'our')
        # TODO intra
        with open(summary_path, 'a') as f:
            f.write(f"intra\n")
        our_dir = algos['our']
        intra_path = os.path.join(our_dir, 'intra_wcet.csv')
        if os.path.exists(intra_path):
            process_and_append(intra_path, summary_path, 'our')

    # TODO 
    
    # 
    with open(summary_path, 'a') as f:
        f.write("\n")

def process_and_append(input_path, output_path, method):
    # CSV
    df = pd.read_csv(input_path)

    df = df.iloc[3:]

    if len(df.columns) == 6:
        df = df.drop(df.columns[[3, 5]], axis=1)
    elif len(df.columns) == 5:
        df = df.drop(df.columns[4], axis=1)

    if df.columns[0] == 'Unnamed: 0':
        df.columns = [''] + list(df.columns[1:])
    
    with open(output_path, 'a') as f:
        df.to_csv(f, index=False)
        f.write("\n")  # 

if __name__ == "__main__":
    root_directory = "/workspaces/llvmta/our_experiment/lab2"  
    process_directories(root_directory)