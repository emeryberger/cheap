import json
import os
import argparse
import sys
import subprocess
from collections import defaultdict

x = json.load(sys.stdin)
trace = x["trace"]

stack_series = defaultdict(list)

parser = argparse.ArgumentParser("cheaper")
parser.add_argument("--progname", help="path to executable")
parser.add_argument("--skip", help="number of stack frames to skip", default=1)

args = parser.parse_args()

if not args.progname:
    parser.print_help()
    sys.exit(-1)
    
depth = args.skip
progname = args.progname # "../memory-management-modeling/mallocbench"

stack_info = {}

# Convert each stack frame into a name and line number
for i in trace:
    for stkaddr in i["stack"][depth:]:
        if stkaddr not in stack_info:
            result = subprocess.run(['addr2line', hex(stkaddr), '-e', progname], stdout=subprocess.PIPE)
            stack_info[stkaddr] = result.stdout.decode('utf-8').strip()
    # print(stack_info[stkaddr])
            
# Separate each trace by its complete stack signature.
for i in trace:
    stk = [stack_info[k] for k in i["stack"][depth:]]
    stack_series[str(stk)].append(i)

for k in stack_series:
    if len(stack_series[k]) < 500:
        continue
    print(k, len(stack_series[k]))
    # Compute total memory footprint
    total_footprint = 0
    for i in stack_series[k]:
        if i["action"] == "M":
            total_footprint += i["size"]
    print("total footprint = " + str(total_footprint))
    # Compute actual footprint
    actual_footprint = 0
    for i in stack_series[k]:
        if i["action"] == "M":
            actual_footprint += i["size"]
        elif i["action"] == "F":
            actual_footprint -= i["size"]
    print("actual footprint = " + str(actual_footprint))
    # Compute score (0 is worst, 1 is best - for region replacement).
    score = 0
    if total_footprint != 0:
        score = actual_footprint / total_footprint
    print("score = " + str(score))
    print("----")
