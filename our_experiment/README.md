## LAB 1

```bash
./lab1.py -s lab1/0516_2c_zw -p -ops options/lab1_l1_82.txt -m zhangw
./lab1.py -s lab1/0516_2c_our -p -ops options/lab1_l1_82.txt -m our
```


### lab1 Data processing

```bash
./post_run.py -s lab1/0516_2c_zw -t lab1/0516_2c_zw -m zhangw
./post_run.py -s lab1/0516_2c_our -t lab1/0516_2c_our -m our
```





## LAB 2
```bash
./lab2.py -s lab2/0515_2c_ly_2 -p -ops options/lab2_l1_82.txt -m liangy -a 2
./lab2.py -s lab2/0515_2c_zw_2 -p -ops options/lab2_l1_82.txt -m zhangw -a 2
./lab2.py -s lab2/0515_2c_our_2 -p -ops options/lab2_l1_82.txt -m our -a 2
```

```bash
./lab2.py -s lab2/0515_2c_ly_4 -p -ops options/lab2_l1_82.txt -m liangy -a 4
./lab2.py -s lab2/0515_2c_zw_4 -p -ops options/lab2_l1_82.txt -m zhangw -a 4
./lab2.py -s lab2/0515_2c_our_4 -p -ops options/lab2_l1_82.txt -m our -a 4
```

```bash
./lab2.py -s lab2/0515_2c_ly_8 -p -ops options/lab2_l1_82.txt -m liangy -a 8
./lab2.py -s lab2/0515_2c_zw_8 -p -ops options/lab2_l1_82.txt -m zhangw -a 8
./lab2.py -s lab2/0515_2c_our_8 -p -ops options/lab2_l1_82.txt -m our -a 8
```
or run it with a helper script   
```bash
./run_lab2.sh
```
### lab2 Data processing

```bash
./post_run.py -s lab2/0515_2c_ly_2 -t lab2/0515_2c_ly_2 -m liangy
./post_run.py -s lab2/0515_2c_zw_2 -t lab2/0515_2c_zw_2 -m zhangw
./post_run.py -s lab2/0515_2c_our_2 -t lab2/0515_2c_our_2 -m our
```

```bash
./post_run.py -s lab2/0515_2c_ly_4 -t lab2/0515_2c_ly_4 -m liangy
./post_run.py -s lab2/0515_2c_zw_4 -t lab2/0515_2c_zw_4 -m zhangw
./post_run.py -s lab2/0515_2c_our_4 -t lab2/0515_2c_our_4 -m our
```

```bash
./post_run.py -s lab2/0515_2c_ly_8 -t lab2/0515_2c_ly_8 -m liangy
./post_run.py -s lab2/0515_2c_zw_8 -t lab2/0515_2c_zw_8 -m zhangw
./post_run.py -s lab2/0515_2c_our_8 -t lab2/0515_2c_our_8 -m our
```

or run it with a helper script   
```bash
./post_run.sh
```