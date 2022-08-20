import fileinput
import re
import sys

print("seed,malloc,occupancy,elapsed")

seed_re = re.compile(r"seed\s+: ([0-9]+)")
malloc_re = re.compile(r"malloc\s+: ([^\s]+)")
occupancy_re = re.compile(r"occupancy\s+: ([0-9\.]+)")
elapsed_re = re.compile(r"\s*([0-9\.]+) seconds time elapsed")

total_fields = 4

fields = 0

for line in fileinput.input():
    line = line.rstrip()
    if m := re.match(seed_re, line):
        seed = int(m.group(1))
        fields += 1
    if m := re.match(malloc_re, line):
        malloc = m.group(1)
        fields += 1
    if m := re.match(occupancy_re, line):
        occupancy = float(m.group(1))
        fields += 1
    if m := re.match(elapsed_re, line):
        elapsed = float(m.group(1))
        fields += 1
    if fields == total_fields:
        print(f"{malloc},{seed},{occupancy},{elapsed}")
        fields = 0
        
        
    
