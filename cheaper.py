import json
import sys
from collections import defaultdict

x = json.load(sys.stdin)
trace = x["trace"]

stack_series = defaultdict(list)

depth = 0

# Separate each trace by its complete stack signature.
for i in trace:
    stack_series[str(i["stack"][depth:])].append(i)

for k in stack_series:
    if len(stack_series[k]) < 100:
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
