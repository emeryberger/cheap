import json
import os
import argparse
import sys
import subprocess
from collections import defaultdict

parser = argparse.ArgumentParser("cheaper")
parser.add_argument("--progname", help="path to executable")
parser.add_argument("--threshold-mallocs", help="threshold allocations to report", default=100)
parser.add_argument("--threshold-score", help="threshold reporting score", default=0.8)
parser.add_argument("--skip", help="number of stack frames to skip", default=1)

args = parser.parse_args()

if not args.progname:
    parser.print_help()
    sys.exit(-1)
    
depth = int(args.skip)
progname = args.progname # "../memory-management-modeling/mallocbench"

x = json.load(sys.stdin)
trace = x["trace"]

stack_series = defaultdict(list)

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
    if len(stack_series[k]) < int(args.threshold_mallocs):
        continue
    # Compute total memory footprint
    total_footprint = 0
    for i in stack_series[k]:
        if i["action"] == "M":
            total_footprint += i["size"]
    # Compute actual footprint
    actual_footprint = 0
    mallocs = set()
    for i in stack_series[k]:
        if i["action"] == "M":
            actual_footprint += i["size"]
            mallocs.add(i["address"])
        elif i["action"] == "F":
            if i["address"] in mallocs:
                actual_footprint -= i["size"]
                mallocs.remove(i["address"])
    # Compute score (0 is worst, 1 is best - for region replacement).
    score = 0
    if total_footprint != 0:
        score = actual_footprint / total_footprint
    if score >= float(args.threshold_score):
        print(k, len(stack_series[k]))
        print("total footprint = " + str(total_footprint))
        print("actual footprint = " + str(actual_footprint))
        print("score = " + str(score))
        print("----")
