"""cheaper.py: identifies where to apply custom allocators"""

# import json
import orjson
import math
import os
import argparse
import sys
import subprocess
from collections import defaultdict

parser = argparse.ArgumentParser("cheaper")
parser.add_argument("--progname", help="path to executable")
parser.add_argument(
    "--threshold-mallocs", help="threshold allocations to report", default=100
)
parser.add_argument("--threshold-score", help="threshold reporting score", default=0.8)
parser.add_argument("--skip", help="number of stack frames to skip", default=1)

args = parser.parse_args()

if not args.progname:
    parser.print_help()
    sys.exit(-1)

depth = int(args.skip)
progname = args.progname

with open("cheaper.out", "r") as f:
    file_contents = f.read()

x = orjson.loads(file_contents)
# x = json.loads(file_contents)

trace = x["trace"]

stack_series = defaultdict(list)

stack_info = {}

# Convert each stack frame into a name and line number
for i in trace:
    for stkaddr in i["stack"][depth:]:
        if stkaddr not in stack_info:
            result = subprocess.run(
                ["addr2line", hex(stkaddr), "-e", progname], stdout=subprocess.PIPE
            )
            stack_info[stkaddr] = result.stdout.decode("utf-8").strip()
    # print(stack_info[stkaddr])

# Separate each trace by its complete stack signature.
for i in trace:
    stk = [stack_info[k] for k in i["stack"][depth:]]
    stack_series[str(stk)].append(i)

# Iterate through each call site, in descending order by number of allocations
for k in sorted(stack_series, key=len, reverse=True):
    sizes = set()
    size_histogram = defaultdict(int)
    if len(stack_series[k]) < int(args.threshold_mallocs):
        # Ignore call sites with too few mallocs
        break # since sorted, the others all have fewer mallocs
    # Compute total memory footprint (excluding frees)
    # We use this to compute a "region score" later.
    total_footprint = 0
    for i in stack_series[k]:
        sizes.add(i["size"])
        size_histogram[i["size"]] += 1
        if i["action"] == "M":
            total_footprint += i["size"]
    # Compute entropy of sizes
    total = len(stack_series[k])
    # print(size_histogram)
    normalized_entropy = -sum(
        [
            float(size_histogram[c])
            / total
            * math.log2(float(size_histogram[c]) / total)
            for c in size_histogram
        ]
    ) / len(size_histogram)
    # print(normalized_entropy)
    # print(len(size_histogram))
    # Compute actual footprint
    actual_footprint = 0
    # set of all thread ids used for malloc/free
    tids = set()
    # set of all (currently) allocated objects from this site
    mallocs = set()
    for i in stack_series[k]:
        tids.add(i["tid"])
        if i["action"] == "M":
            actual_footprint += i["size"]
            mallocs.add(i["address"])
        elif i["action"] == "F":
            if i["address"] in mallocs:
                actual_footprint -= i["size"]
                mallocs.remove(i["address"])
    # Compute region_score (0 is worst, 1 is best - for region replacement).
    region_score = 0
    if total_footprint != 0:
        region_score = actual_footprint / total_footprint
    if region_score >= float(args.threshold_score):
        print(k, "allocations = ", len(stack_series[k]))
        print("candidate for region allocation")
        print("* region score = " + str(region_score), "(1 is best)")
        if len(tids) == 1:
            print("* single-threaded (no lock needed)")
        size_list = list(sizes)
        size_list.sort()
        if len(sizes) == 1:
            print("* all the same size (", size_list[0], ")")
        else:
            print(str(len(sizes)) + " different sizes = ", size_list)
            print("size entropy = ", normalized_entropy)
        print("total footprint = " + str(total_footprint))
        print("actual footprint = " + str(actual_footprint))
        print("----")
