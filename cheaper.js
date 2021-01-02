'use strict';
const {
    ArgumentParser,
    RawTextHelpFormatter
} = require('argparse');
const fs = require('fs');
const {
    execSync
} = require("child_process");

parse()

function parse() {
    const parser = new ArgumentParser({
        description: 'Cheaper: finds where you should use a custom heap.',
        prog: 'cheaper',
        formatter_class: RawTextHelpFormatter,
    });

    parser.add_argument('--progname', {
        help: 'path to executable',
        required: true
    });
    parser.add_argument('--jsonfile', {
        help: 'json to read',
        required: true
    });
    parser.add_argument('--threshold-mallocs', {
        help: 'threshold allocations to report',
        default: 100
    });
    parser.add_argument('--threshold-score', {
        help: 'threshold reporting score',
        default: 0.8
    });
    parser.add_argument('--skip', {
        help: 'number of stack frames to skip',
        default: 0
    });
    parser.add_argument('--depth', {
        help: 'total number of stack frames to use (from top)',
        default: 5
    });
    const args = parser.parse_args();
    if (args.progname === undefined)
        return -1;
    return runIt(args.jsonfile, args.progname, args.depth, args.threshold_mallocs, args.threshold_score);
}

//JS has no built in has function, so if I were to put a hash I would put it here
function hash(input) {
    return input;
}

function processFile(data, progname, depth, threshold_mallocs, threshold_score) {
    
    const trace = JSON.parse(data).trace;
    console.log("Done parsing.");
        let analyzed = [];
        for (let d = 1; d < depth; d++) {
            analyzed = analyzed.concat(process_trace(trace, progname, d, threshold_mallocs, threshold_score));
        }
        //Sort in reverse order by region score * number of allocations
        analyzed = analyzed.sort((a, b) => {
	    return (b[0].region_score * b[0].allocs * b[0].stack.length) - (a[0].region_score * a[0].allocs * a[0].stack.length)
	});
        analyzed.forEach(l => {
            l.forEach(item => {
                if (item.stack) {
                    item.stack.forEach(stk => console.log(stk));
                    console.log("-----");
                    console.log("region score = ", item.region_score);
                    console.log("number of allocs = ", item.allocs);
                    console.log("sizes = ", item.sizes);
                    console.log("threads = ", item.threads);
                    console.log("=====");
                }
            });
        });
}

function runIt(jsonfile, progname, depth, threshold_mallocs, threshold_score) {
    if (!fs.existsSync(jsonfile))
        return -2;
    fs.readFile(jsonfile, (err, data) =>
		processFile(data, progname, depth, threshold_mallocs, threshold_score));
}

function convertStackFrame(item, depth, progname, stack_info) {
    item.stack.slice(-depth).forEach(stkaddr => {
        if (!(stkaddr in stack_info)) {
            const out = execSync("addr2line 0x" + stkaddr.toString(16) + " -C -e " + progname).toString('utf8');
            stack_info[stkaddr] = out.includes('??') ? 'BAD' : out.replace(/^\s+|\s+$/gm, '');
        }
    })
}

function separateTrace(item, depth, stack_info, stack_series)
{
    const stk = item.stack.slice(-depth).map(k => stack_info[k]).filter(s => s != 'BAD');
    const stkstr = "['" + stk.join("', '") + "']";
    if (stk.length) {
        if (!Array.isArray(stack_series[stkstr])) stack_series[stkstr] = [];
        stack_series[stkstr].push(item);
    }
}

function process_trace(trace, progname, depth, threshold_mallocs, threshold_score) {
    let stack_series = {};
    const stack_info = {};
    // Convert each stack frame into a name and line number
    for (let i of trace) {
	convertStackFrame(i, depth, progname, stack_info);
    }
    // Separate each trace by its complete stack signature.
    for (let i of trace) {
	separateTrace(i, depth, stack_info, stack_series);
    }
    // Iterate through each call site.
    const analyzed = [];
    Object.keys(stack_series).forEach(k =>
        analyzed.push(analyze(stack_series[k], k, progname, depth, threshold_mallocs, threshold_score))
    );
    return analyzed.filter(list => list.length);
}

function analyze(allocs, stackstr, progname, depth, threshold_mallocs, threshold_score) {
    //Analyze a trace of allocations and frees.
    const analyzed_list = [];
    if (allocs.length < parseInt(threshold_mallocs)) {
        //Ignore call sites with too few mallocs
        return analyzed_list;
    }
    //The set of sizes of allocated objects.
    const sizes = new Set();
    //A histogram of the # of objects allocated of each size.
    const size_histogram = {};
    let actual_footprint = 0; //mallocs - frees
    let peak_footprint = 0; //max actual_footprint
    let peak_footprint_index = 0; //index of alloc w/max footprint
    let nofree_footprint = 0; //sum(mallocs)
    //set of all thread ids used for malloc/free
    const tids = new Set();
    //set of all (currently) allocated objects from this site
    const mallocs = new Set();
    let num_allocs = 0;
    const utilization = 0;
    allocs.forEach((i, index) => {
        sizes.add(i.size);
        size_histogram[i.size]++;
        tids.add(i.tid);
        if (i.action === 'M') {
            num_allocs++;
            //Compute actual footprint (taking into account mallocs and frees).
            actual_footprint += i.size;
            if (actual_footprint > peak_footprint) {
                peak_footprint = actual_footprint;
                peak_footprint_index = index;
            }
            // Compute total 'no-free' memory footprint (excluding frees) This
            // is how much memory would be consumed if we didn't free anything
            // until the end (as with regions/arenas). We use this to compute a
            // "region score" later.
            nofree_footprint += i.size;
            // Record the malloc so we can check it when freed.
            mallocs.add(i.address);
        } else if (i.action === 'F') {
            if (mallocs.has(i.address)) {
                // Only reclaim memory that we have already allocated
                // (others are frees to other call sites).
                actual_footprint -= i.size;
                mallocs.delete(i.address);
            } else {
                // print(mallocs)
                // print(str(i["address"]) + " not found")
            }
        }
    })
    // Recompute utilization
    // frag = Cheaper.utilization(allocs, peak_footprint_index)
    // Compute entropy of sizes
    const total = allocs.length;
    const normalized_entropy = -Object.keys(size_histogram).reduce((a, e) =>
        (e / total * Math.log2(e / total)) + a
    ) / size_histogram.length;
    // Compute region_score (0 is worst, 1 is best - for region replacement).
    let region_score = 0;
    if (nofree_footprint != 0)
        region_score = peak_footprint / nofree_footprint;
    if (region_score >= threshold_score) {
        const stk = eval(stackstr);
        const output = {
            "stack": stk,
            "allocs": num_allocs,
            "region_score": region_score,
            "threads": tids,
            "sizes": sizes,
            "size_entropy": normalized_entropy,
            "peak_footprint": peak_footprint,
            "nofree_footprint": nofree_footprint,
        };
        analyzed_list.push(output);
    }
    return analyzed_list;
}
