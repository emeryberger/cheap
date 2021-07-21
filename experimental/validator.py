import re
import fileinput

matcher = r"\d+\s+[a-z\_\.]+\s+0x[0-9a-f]+\s+([a-zA-Z\_0-9]+)"
f = re.compile(matcher)

test_str = "0   libhoard.dylib                      0x0000000113c480af xxmalloc + 159"
r = f.match(test_str)

print(re.findall(f, test_str))

print("WOOT")

state = 0

for count, line in enumerate(fileinput.input()):
    results = re.findall(f, line)
    if not results:
        continue
    # Look for mallocs called by any Bloomberg function that isn't
    # just the wrapper around new/delete/malloc/free, IF they aren't
    # intercepted by a call to a SimPool or SimRegion.
    if state == 0 and results[0] == "xxmalloc":
        state = 1
    elif state == 1 and ("Bloomberg" in results[0] and "NewDeleteAllocator" in results[0] or "MallocFreeAllocator" in results[0]):
        # print("GOOD " + str(count))
        state = 0
    elif state == 1 and ("SimRegion" in results[0] or "SimPool" in results[0]):
        state = 2
    elif state == 1 and ("Bloomberg" in results[0] and "NewDeleteAllocator" not in results[0] and "MallocFreeAllocator" not in results[0]):
        print("CRAP " + str(count))
        state = 0
    elif state == 2 and ("Bloomberg" in results[0] and "NewDeleteAllocator" not in results[0] and "MallocFreeAllocator" not in results[0]):
        # print("GOOD " + str(count))
        state = 0
        
        
