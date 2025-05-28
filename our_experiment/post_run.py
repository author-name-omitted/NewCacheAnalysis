#!/usr/bin/env python3
# flake8:noqa

import os
import argparse
import csv
from collections import defaultdict, OrderedDict

def parse_rwinfo(filepath):
    """ RWInfo.txt """
    stats = defaultdict(lambda: defaultdict(lambda: defaultdict(int)))
    with open(filepath, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 4:
                func = parts[0]
                access_type = parts[1]
                category = parts[2]
                count = int(parts[3])
                stats[func][access_type][category] = count
    return stats

def parse_result_txt(filepath, m_method):
    """ Result.txt """
    intra_results = {}  #  intra 
    inter_results = {}  #  inter 

    with open(filepath, 'r') as f:
        if m_method == "liangy" or m_method == "none":
            funcs = []
            times = []
            for line in f:
                parts = line.strip().split()
                if len(parts) == 3 and parts[1] == "intra":
                    funcs.append(parts[0])
                    times.append(int(parts[2]))
            if len(funcs)<2:
                print(f"{filepath} not enough function in Result.txt")
                # exit(1)
            else: # FIXME
                if funcs[0] not in intra_results:
                    intra_results[funcs[0]] = {}
                if funcs[1] not in intra_results:
                    intra_results[funcs[1]] = {}
                intra_results[funcs[0]][funcs[1]] = times[0]
                intra_results[funcs[1]][funcs[0]] = times[1]
        else:
            funcs = []
            times = []
            for line in f:
                parts = line.strip().split()
                if len(parts) == 3 and parts[1] == "intra":
                    # : func intra time
                    func = parts[0]
                    time = int(parts[2])
                    funcs.append(func)
                    times.append(time)
                elif len(parts) == 4 and parts[1] == "inter":
                    # : func1 inter func2 time
                    func1 = parts[0]  # 
                    func2 = parts[2]  # 
                    time = int(parts[3])

                    if func1 not in inter_results:
                        inter_results[func1] = {}
                    inter_results[func1][func2] = time
            if len(funcs)!=2:
                print(f"{filepath} functions in Result.txt uncorrect")
                # exit(1) 
            else: # FIXME
                if funcs[0] not in intra_results:
                    intra_results[funcs[0]] = {}
                if funcs[1] not in intra_results:
                    intra_results[funcs[1]] = {}
                intra_results[funcs[0]][funcs[1]] = times[0]
                intra_results[funcs[1]][funcs[0]] = times[1]      

    return intra_results, inter_results

# _main
def extract_func_name(raw_name):
    # 
    first_underscore = raw_name.find('_')
    name_after_number = raw_name[first_underscore+1:]
    # "_main"
    if name_after_number.endswith('_main'):
        func_name = name_after_number[:-5]
    else:
        func_name = name_after_number
    return func_name + "_main"


def parse_statistics_txt(filepath):
    """ Statistics.txt ，"""
    stats = {
        'complete_analysis': 0,
        'intra_times': {},
        'inter_times': {}
    }

    current_measurement = None
    current_id = None

    with open(filepath, 'r') as f:
        funcs = []
        times = []
        # print_stats(stats)
        for line in f:
            line = line.strip()

            if line == '<measurement>':
                current_measurement = {}
            elif line == '</measurement>' and current_id:
                if current_id == 'Complete Analysis':
                    stats['complete_analysis'] = current_measurement.get('time', 0)
                elif '_intra' in current_id:
                    if current_id.endswith('_intra'):
                        functions_part = current_id[:-6]  #  '_intra'
                        func_name = extract_func_name(functions_part)
                        funcs.append(func_name)
                        times.append(current_measurement.get('time', 0))
                        # stats['intra_times'][func_name] = current_measurement.get('time', 0)

                elif '_inter' in current_id:
                    if current_id.endswith('_inter'):
                        functions_part = current_id[:-6]  #  '_inter'
                        first_sep = functions_part.find('_main_')
                        if first_sep != -1:
                            func1_raw = functions_part[:first_sep+5]  # _main
                            func2_raw = functions_part[first_sep+6:]

                            func1 = extract_func_name(func1_raw)
                            func2 = extract_func_name(func2_raw)

                            if func1 not in stats['inter_times']:
                                stats['inter_times'][func1] = {}
                            stats['inter_times'][func1][func2] = current_measurement.get('time', 0)

                current_id = None

            elif line.startswith('<id>') and line.endswith('</id>'):
                current_id = line[4:-5]
            elif line.startswith('<time>') and line.endswith('</time>'):
                current_measurement['time'] = float(line[6:-7])

        if funcs[0] not in stats['intra_times']:
            stats['intra_times'][funcs[0]] = {}
        if funcs[1] not in stats['intra_times']:
            stats['intra_times'][funcs[1]] = {}
        stats['intra_times'][funcs[0]][funcs[1]] = times[0]
        stats['intra_times'][funcs[1]][funcs[0]] = times[1]
        # print_stats(stats)
    return stats

def print_stats(stats):
    print(f"Complete Analysis: {stats['complete_analysis']}")
    print("\nIntra Times:")
    # for func, time in stats['intra_times'].items():
    #     print(f"  {func}: {time}")
    for func1, inner in stats['intra_times'].items():
        for func2, time in inner.items():
            print(f"  {func1} -> {func2}: {time}")
    print("\nInter Times:")
    for func1, inner in stats['inter_times'].items():
        for func2, time in inner.items():
            print(f"  {func1} -> {func2}: {time}")



def scan_directory(root_dir, m_method):
    """"""
    all_stats = defaultdict(lambda: defaultdict(lambda: defaultdict(int)))
    all_stats_u = defaultdict(lambda: defaultdict(lambda: defaultdict(int)))
    intra_times = {}  #  intra 
    inter_times = {}  #  inter 
    analysis_times = defaultdict(dict)
    #  Statistics.txt 
    all_stat_data = {
        'complete_analysis': [],  # 
        'intra_times': defaultdict(lambda: defaultdict(int)),  #  intra ，
        'inter_times': defaultdict(lambda: defaultdict(int))  #  inter ，
    }
    
    # 
    all_functions = set()
    
    for subdir in os.listdir(root_dir):
        subdir_path = os.path.join(root_dir, subdir)
        if not os.path.isdir(subdir_path):
            continue
        
        build_dir = os.path.join(subdir_path, "build")
        if not os.path.exists(build_dir):
            continue
        
        # RWInfo
        rwinfo_path = os.path.join(build_dir, "RWInfo.txt")
        if os.path.exists(rwinfo_path):
            stats = parse_rwinfo(rwinfo_path)
            for func in stats:
                all_functions.add(func)
                for access_type in stats[func]:
                    for category in stats[func][access_type]:
                        all_stats[func][access_type][category] = stats[func][access_type][category]
        
        # RWInfo_u
        rwinfo_path_u = os.path.join(build_dir, "RWInfo_u.txt")
        if os.path.exists(rwinfo_path_u):
            stats = parse_rwinfo(rwinfo_path_u)
            for func in stats:
                all_functions.add(func)
                for access_type in stats[func]:
                    for category in stats[func][access_type]:
                        all_stats_u[func][access_type][category] = stats[func][access_type][category]
        
        # Result
        result_path = os.path.join(build_dir, "Result.txt")
        if os.path.exists(result_path):
            intra_results, inter_results = parse_result_txt(result_path, m_method)
            
            # 
            for func in intra_results:
                all_functions.add(func)
                for interfering_func in intra_results[func]:
                    all_functions.add(interfering_func)
            
            for func in inter_results:
                all_functions.add(func)
                for interfering_func in inter_results[func]:
                    all_functions.add(interfering_func)
            
            #  intra  
            for func, interfering_funcs in intra_results.items():
                if func not in intra_times:
                    intra_times[func] = {}

                for interfering_func, time in interfering_funcs.items():
                    intra_times[func][interfering_func] = time
            
            #  inter 
            for func, interfering_funcs in inter_results.items():
                if func not in inter_times:
                    inter_times[func] = {}
                
                for interfering_func, time in interfering_funcs.items():
                    inter_times[func][interfering_func] = time

        # Statistics
        stat_path = os.path.join(build_dir, "Statistics.txt")
        if os.path.exists(stat_path):
            try:
                stat_times = parse_statistics_txt(stat_path)
                
                # 
                if 'complete_analysis' in stat_times:
                    all_stat_data['complete_analysis'] = stat_times['complete_analysis']
                
                #  intra 
                # for func, time in stat_times['intra_times'].items():
                #     all_functions.add(func)
                #     all_stat_data['intra_times'][func].append(time)
                
                for func1, interfering_funcs in stat_times['intra_times'].items():
                    all_functions.add(func1)
                    for func2, time in interfering_funcs.items():
                        all_functions.add(func2)
                        all_stat_data['intra_times'][func1][func2] = time
                
                #  inter 
                for func1, interfering_funcs in stat_times['inter_times'].items():
                    all_functions.add(func1)
                    for func2, time in interfering_funcs.items():
                        all_functions.add(func2)
                        all_stat_data['inter_times'][func1][func2] = time
                
            except Exception as e:
                print(f" {stat_path} : {e}")
                continue
        
        # 
    # （、）
    # stat_data = {
    #     'complete_analysis': sum(all_stat_data['complete_analysis']) / len(all_stat_data['complete_analysis']) if all_stat_data['complete_analysis'] else 0,
    #     'intra_times': {},
    #     'inter_times': {}
    # }
    
    #  intra 
    # for func, times in all_stat_data['intra_times'].items():
    #     stat_data['intra_times'][func] = sum(times) / len(times) if times else 0
    
    #  inter 
    # for func1, interfering_funcs in all_stat_data['inter_times'].items():
    #     if func1 not in stat_data['inter_times']:
    #         stat_data['inter_times'][func1] = {}
        
    #     for func2, times in interfering_funcs.items():
    #         stat_data['inter_times'][func1][func2] = sum(times) / len(times) if times else 0

    # print_all_stat_data(all_stat_data)
    return all_stats, all_stats_u, intra_times, inter_times, all_functions, all_stat_data

# debug
from collections import defaultdict

def print_all_stat_data(all_stat_data):
    print("Complete Analysis Times:")
    for time in all_stat_data['complete_analysis']:
        print(f"  {time}")

    print("\nIntra Times:")
    for func1, inner in all_stat_data['intra_times'].items():
        for func2, times in inner.items():
            print(f"  {func1} -> {func2}: {times}")

    print("\nInter Times:")
    for func1, inner in all_stat_data['inter_times'].items():
        for func2, times in inner.items():
            print(f"  {func1} -> {func2}: {times}")

# 
# print_all_stat_data(all_stat_data)


def generate_markdown(stats, output_file):
    """ Markdown """
    with open(output_file, 'w') as f:
        f.write("# \n\n")
        f.write("|  |  | Hit | L2Hit | L2PS | L2Miss |\n")
        f.write("|--------|----------|-----|-------|------|--------|\n")
        
        for func in sorted(stats.keys()):
            for access_type in sorted(stats[func].keys()):
                hits = stats[func][access_type].get('Hit', 0)
                l2hits = stats[func][access_type].get('L2Hit', 0)
                l2pss = stats[func][access_type].get('L2PS', 0)
                l2misses = stats[func][access_type].get('L2Miss', 0)
                
                f.write(f"| {func} | {access_type} | {hits} | {l2hits} | {l2pss} | {l2misses} |\n")

def generate_interference_csv(intra_times, inter_times, all_functions, output_file_interference, output_file_total, output_file_intra):
    """CSVCSV"""
    # 
    functions = sorted(all_functions)
    
    # CSV
    with open(output_file_interference, 'w', newline='') as f:
        writer = csv.writer(f)
        # （，）
        header = [''] + functions
        writer.writerow(header)
        
        # 
        for func1 in functions:
            row = [func1]  # 
            for func2 in functions:
                if func1 in inter_times and func2 in inter_times[func1]:
                    # 
                    row.append(inter_times[func1][func2])
                else:
                    # 
                    row.append(0)
            writer.writerow(row)
    
    # +intraCSV
    with open(output_file_total, 'w', newline='') as f:
        writer = csv.writer(f)
        # 
        header = [''] + functions
        writer.writerow(header)
        
        # 
        for func1 in functions:
            row = [func1]  # 
            for func2 in functions:
                if func1 in inter_times and func2 in inter_times[func1]:
                    # ，intra
                    row.append(inter_times[func1][func2] + intra_times[func1][func2])
                else:
                    # ，intra
                    row.append(0)
            writer.writerow(row)

    # intraCSV
    with open(output_file_intra, 'w', newline='') as f:
        writer = csv.writer(f)
        # 
        header = [''] + functions
        writer.writerow(header)
        
        #  
        for func1 in functions:
            row = [func1]  # 
            for func2 in functions:
                if func1 in intra_times and func2 in intra_times[func1]:
                    # ，intra
                    row.append(intra_times[func1][func2])
                else:
                    row.append(intra_times.get(func1, {}).get(func2, 0))
            writer.writerow(row)

def generate_statistics_csv(stats, all_functions, output_file_inter, output_file_total):
    """CSVCSV"""
    intra_times = stats['intra_times']
    inter_times = stats['inter_times']
    
    # 
    functions = sorted(all_functions)
    
    # interCSV
    with open(output_file_inter, 'w', newline='') as f:
        writer = csv.writer(f)
        # 
        header = [''] + functions
        writer.writerow(header)
        
        # 
        for func1 in functions:
            row = [func1]  # 
            for func2 in functions:
                if func1 in inter_times and func2 in inter_times[func1]:
                    # 
                    row.append(inter_times[func1][func2])
                else:
                    # 
                    row.append(0)
            writer.writerow(row)
    
    # inter+intraCSV
    with open(output_file_total, 'w', newline='') as f:
        writer = csv.writer(f)
        # 
        header = [''] + functions
        writer.writerow(header)
        
        # 
        for func1 in functions:
            row = [func1]  # 
            for func2 in functions:
                if func1 in inter_times and func2 in inter_times[func1]:
                    # ，intra
                    inter_time = inter_times[func1][func2]
                    func1_intra = intra_times[func1][func2]
                    func2_intra = intra_times[func2][func1]
                    row.append(inter_time + func1_intra + func2_intra)
                else:
                    # ，intra
                    func1_intra = intra_times[func1][func2]
                    func2_intra = intra_times[func2][func1]
                    row.append(func1_intra + func2_intra)
            writer.writerow(row)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="")
    parser.add_argument('-s', '--src', type=str, required=True, help='， ./path/to/test')
    parser.add_argument('-t', '--out', type=str, required=True, help='， ./path/to/output')
    parser.add_argument('-m', "--multicore", choices=["zhangw", "liangy", "our", "none"], help="")
    args = parser.parse_args()
    
    root_directory = args.src
    output_dir = args.out
    
    if not os.path.isdir(root_directory):
        print(f":  '{root_directory}' ")
        exit(1)
    
    # 
    os.makedirs(output_dir, exist_ok=True)
    
    # 
    output_md = os.path.join(output_dir, "rwinfo_summary.md")
    output_md_u = os.path.join(output_dir, "rwinfo_summary_u.md")
    output_interference_csv = os.path.join(output_dir, "wceet.csv")
    output_total_csv = os.path.join(output_dir, "total_wcet.csv")
    output_intra_csv = os.path.join(output_dir, "intra_wcet.csv")
    output_stat_inter_csv = os.path.join(output_dir, "wceet_runtime.csv")
    output_stat_total_csv = os.path.join(output_dir, "total_runtime.csv")
    
    # 
    all_stats, all_stats_u, intra_times, inter_times, all_functions, stat_data = scan_directory(root_directory, args.multicore)
    
    #  Markdown 
    # generate_markdown(all_stats, output_md)
    # generate_markdown(all_stats_u, output_md_u)
    
    # ResultCSV
    generate_interference_csv(intra_times, inter_times, all_functions, 
                             output_interference_csv, output_total_csv, 
                             output_intra_csv)
    
    # Statistics.txt，CSV
    # stat_data = None
    # for subdir in os.listdir(root_directory):
    #     subdir_path = os.path.join(root_directory, subdir)
    #     if not os.path.isdir(subdir_path):
    #         continue
        
    #     build_dir = os.path.join(subdir_path, "build")
    #     if not os.path.exists(build_dir):
    #         continue
        
    #     stat_path = os.path.join(build_dir, "Statistics.txt")
    #     if os.path.exists(stat_path):
    #         try:
    #             stat_data = parse_statistics_txt(stat_path)
    #             # all_functions
    #             for func in stat_data['intra_times']:
    #                 all_functions.add(func)
    #             for func1 in stat_data['inter_times']:
    #                 all_functions.add(func1)
    #                 for func2 in stat_data['inter_times'][func1]:
    #                     all_functions.add(func2)
    #             break  # Statistics.txt
    #         except Exception as e:
    #             print(f" {stat_path} : {e}")
    #             continue
    
    # Statistics.txt，CSV
    if stat_data:
        generate_statistics_csv(stat_data, all_functions, 
                               output_stat_inter_csv, output_stat_total_csv)
        print(f"inter analysis runtime: {output_stat_inter_csv}")
        print(f"total analysis runtime: {output_stat_total_csv}")
    
    # print(f"cache access infomation : {output_md}")
    print(f"inter analysis result: {output_interference_csv}")
    print(f"intra analysis result: {output_intra_csv}")
    print(f"total result: {output_total_csv}")