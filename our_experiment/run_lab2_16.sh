#!/bin/bash

TEST_NUM="0510"
# （）
echo "=====  ====="
./lab2.py -s lab2/${TEST_NUM}_2c_ly_16 -p -ops options/lab2_l1_82.txt -m liangy -a 16
echo "=====  ====="

# 8（）
commands=(
    "./lab2.py -s lab2/${TEST_NUM}_2c_zw_16 -p -ops options/lab2_l1_82.txt -m zhangw -a 16"
    "./lab2.py -s lab2/${TEST_NUM}_2c_our_16 -p -ops options/lab2_l1_82.txt -m our -a 16"
    "./lab2.py -s lab2/${TEST_NUM}_2c_ly_10 -p -ops options/lab2_l1_82.txt -m liangy -a 10"
    "./lab2.py -s lab2/${TEST_NUM}_2c_zw_10 -p -ops options/lab2_l1_82.txt -m zhangw -a 10"
    "./lab2.py -s lab2/${TEST_NUM}_2c_our_10 -p -ops options/lab2_l1_82.txt -m our -a 10"
    "./lab2.py -s lab2/${TEST_NUM}_2c_ly_12 -p -ops options/lab2_l1_82.txt -m liangy -a 12"
    "./lab2.py -s lab2/${TEST_NUM}_2c_zw_12 -p -ops options/lab2_l1_82.txt -m zhangw -a 12"
    "./lab2.py -s lab2/${TEST_NUM}_2c_our_12 -p -ops options/lab2_l1_82.txt -m our -a 12"
    # "./lab2.py -s lab2/${TEST_NUM}_2c_ly_2 -p -ops options/lab2_l1_82.txt -m liangy -a 2"
    # "./lab2.py -s lab2/${TEST_NUM}_2c_zw_2 -p -ops options/lab2_l1_82.txt -m zhangw -a 2"
    # "./lab2.py -s lab2/${TEST_NUM}_2c_our_2 -p -ops options/lab2_l1_82.txt -m our -a 2"
    # "./lab2.py -s lab2/${TEST_NUM}_2c_ly_4 -p -ops options/lab2_l1_82.txt -m liangy -a 4"
    # "./lab2.py -s lab2/${TEST_NUM}_2c_zw_4 -p -ops options/lab2_l1_82.txt -m zhangw -a 4"
    # "./lab2.py -s lab2/${TEST_NUM}_2c_our_4 -p -ops options/lab2_l1_82.txt -m our -a 4"
    # "./lab2.py -s lab2/${TEST_NUM}_2c_ly_8 -p -ops options/lab2_l1_82.txt -m liangy -a 8"
    # "./lab2.py -s lab2/${TEST_NUM}_2c_zw_8 -p -ops options/lab2_l1_82.txt -m zhangw -a 8"
    # "./lab2.py -s lab2/${TEST_NUM}_2c_our_8 -p -ops options/lab2_l1_82.txt -m our -a 8"
)

# ，
for i in "${!commands[@]}"; do
    eval "${commands[$i]}" > "lab2_output_$i.log" 2>&1 &
    echo " $i: ${commands[$i]}"
done

echo "， lab2_output_[0-8].log "