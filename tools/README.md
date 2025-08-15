# callgraph creator

callgraph_creator receives a trace file created by DataCrumbs and generates a call graph of its functions. 

### usage

```
Usage: ./callgraph_creator <filepath> [options]

Analyzes a performance trace file to build a call graph.

Options:
  -h, --help                     Show this help message and exit.
  -p, --show-percentage          Display the percentage of time each function took (text mode only).
  -t, --time-metric <type>       Metric for time display (text mode only). <type> can be 'inclusive' or 'exclusive'.
  -f, --focus-function <name>    Focus the output on all instances of a specific function.
  -s, --force-sort               Force the program to sort the trace data if it might be out of order.
  -a, --aggregate                Aggregate children with the same name into a single entry.
  -o, --output-format <format>   Specify the output format. <format> can be:
                                   'text' (default): Human-readable call tree.
                                   'dot': DOT language file for Graphviz.
      --min-percent-root <num>   Hide root functions that are less than <num>% of the total trace time.
      --min-percent-children <num> Hide child functions that are less than <num>% of their parent's time.
```
### samples

Example outputs are in the example folder. The traces used for examples are created by running VPIC-IO. The text output was created by using the `focus-function H5Dwrite` functions, and the picture was created using `--min-percent-root 0.001 --min-percent-children 5`.
