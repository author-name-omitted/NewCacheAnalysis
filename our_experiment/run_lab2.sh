#!/bin/bash

TEST_NUM="0515"

echo "=========="
./lab2.py -s lab2/${TEST_NUM}_2c_ly_2 -p -ops options/lab2_l1_82.txt -m liangy -a 2
echo "=========="

commands=(
    "./lab2.py -s lab2/${TEST_NUM}_2c_zw_2 -p -ops options/lab2_l1_82.txt -m zhangw -a 2"
    "./lab2.py -s lab2/${TEST_NUM}_2c_our_2 -p -ops options/lab2_l1_82.txt -m our -a 2"
    "./lab2.py -s lab2/${TEST_NUM}_2c_ly_4 -p -ops options/lab2_l1_82.txt -m liangy -a 4"
    "./lab2.py -s lab2/${TEST_NUM}_2c_zw_4 -p -ops options/lab2_l1_82.txt -m zhangw -a 4"
    "./lab2.py -s lab2/${TEST_NUM}_2c_our_4 -p -ops options/lab2_l1_82.txt -m our -a 4"
    "./lab2.py -s lab2/${TEST_NUM}_2c_ly_8 -p -ops options/lab2_l1_82.txt -m liangy -a 8"
    "./lab2.py -s lab2/${TEST_NUM}_2c_zw_8 -p -ops options/lab2_l1_82.txt -m zhangw -a 8"
    "./lab2.py -s lab2/${TEST_NUM}_2c_our_8 -p -ops options/lab2_l1_82.txt -m our -a 8"
)

for i in "${!commands[@]}"; do
    eval "${commands[$i]}" > "lab2_output_$i.log" 2>&1 &
    echo "cmd$i: ${commands[$i]}"
done

echo "see result in lab2_output_[0-8].log "