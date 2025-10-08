#include <ctype.h>
#include <getopt.h>
#include <json-c/json.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>

// --- Type Definitions and Global Structures ---

typedef enum { FORMAT_TEXT, FORMAT_DOT } OutputFormat;

typedef struct Node {
  char* name;
  long long ts;
  long long dur;
  long long ts_end;
  long long exclusive_dur;
  struct Node** children;
  int children_count;
  int children_capacity;
  int count = 1;
  long long max;
  long long min;
  int depth = 0;
} Node;

// Global arguments parsed from command line
typedef struct Args {
  int show_percentage;
  int is_exclusive_metric;
  int force_sort;
  double min_percent_root;
  double min_percent_children;
  OutputFormat output_format;
  char* focus_function;
  char* filepath;
  int aggregate;
  int depth_cut = 20;
} Args;

void print_help(const char* prog_name) {
  printf("Usage: %s <filepath> [options]\n\n", prog_name);
  printf("Analyzes a performance trace file to build a call graph.\n\n");
  printf("Options:\n");
  printf("  -h, --help                     Show this help message and exit.\n");
  printf(
      "  -p, --show-percentage          Display the percentage of time each function took (text "
      "mode only).\n");
  printf(
      "  -a, --aggregate                Aggregate repetitive functions under the same parent into "
      "one entry.\n");
  printf(
      "  -t, --time-metric <type>       Metric for time display. <type> can be 'inclusive' or "
      "'exclusive'.\n");
  printf(
      "  -f, --focus-function <name>    Focus the output on all instances of a specific "
      "function.\n");
  printf(
      "  -s, --force-sort               Force sorting of trace data (useful if out of order).\n");
  printf("  -o, --output-format <format>   Specify the output format. <format> can be:\n");
  printf("                                   'text' (default): Human-readable call tree.\n");
  printf("                                   'dot': DOT language file for Graphviz.\n");
  printf(
      "  -d, --depth_cut <num>          Set the maximum depth for the call tree (default: 20).\n");
  printf(
      "      --min-percent-root <num>   Hide root functions consuming less than <num>%% of total "
      "time.\n");
  printf(
      "      --min-percent-children <num> Hide child functions consuming less than <num>%% of "
      "parent's time.\n");
}

void calculate_exclusive_times(Node** nodes, int count) {
  for (int i = 0; i < count; i++) {
    Node* node = nodes[i];
    long long children_total_dur = 0;
    for (int j = 0; j < node->children_count; j++) {
      children_total_dur += node->children[j]->dur;
    }
    node->exclusive_dur = node->dur - children_total_dur;
    if (node->children_count > 0) {
      calculate_exclusive_times(node->children, node->children_count);
    }
  }
}

void find_nodes_by_name(Node** nodes, int count, const char* name, Node*** found_nodes,
                        int* found_count) {
  for (int i = 0; i < count; i++) {
    if (strcmp(nodes[i]->name, name) == 0) {
      *found_nodes = (Node**)realloc(*found_nodes, (*found_count + 1) * sizeof(Node*));
      (*found_nodes)[*found_count] = nodes[i];
      (*found_count)++;
    }
    if (nodes[i]->children_count > 0) {
      find_nodes_by_name(nodes[i]->children, nodes[i]->children_count, name, found_nodes,
                         found_count);
    }
  }
}
// For events starting at the same time, process the larger (parent) one first
int compare_nodes(const void* a, const void* b) {
  Node* nodeA = *(Node**)a;
  Node* nodeB = *(Node**)b;
  if (nodeA->ts < nodeB->ts) return -1;
  if (nodeA->ts > nodeB->ts) return 1;
  if (nodeA->dur > nodeB->dur) return -1;
  if (nodeA->dur < nodeB->dur) return 1;
  return 0;
}

void sanitize_for_graphing(const char* input, char* output, size_t size) {
  size_t i = 0;
  for (i = 0; i < size - 1 && input[i] != '\0'; i++) {
    if (input[i] == ';' || isspace(input[i]) || input[i] == '"' || input[i] == '\\') {
      output[i] = '_';
    } else {
      output[i] = input[i];
    }
  }
  output[i] = '\0';
}

void print_tree(Node** nodes, int count, Args* args, long long total_run_time, const char* prefix,
                long long parent_inclusive_dur, const char* stack_prefix) {
  for (int i = 0; i < count; i++) {
    Node* node = nodes[i];

    double percentage = 0.0;
    if (parent_inclusive_dur == -1) {  // Root node
      if (total_run_time > 0) percentage = ((double)node->dur / total_run_time) * 100.0;
      if (args->min_percent_root > 0 && percentage < args->min_percent_root) {
        continue;
      }
    } else {  // Child node
      if (parent_inclusive_dur > 0) percentage = ((double)node->dur / parent_inclusive_dur) * 100.0;
      if (args->min_percent_children > 0 && percentage < args->min_percent_children) {
        continue;
      }
    }

    if (args->output_format == FORMAT_TEXT) {
      const char* connector = (i == count - 1) ? "└── " : "├── ";
      printf("%s%s%s (dur: %lld [%s], depth: %d)", prefix, connector, node->name,
             args->is_exclusive_metric ? node->exclusive_dur : node->dur,
             args->is_exclusive_metric ? "exclusive" : "inclusive", node->depth);

      if (args->aggregate && node->count > 1) {
        printf(" (count: %d, min: %lld, max: %lld, avg: %lld)", node->count, node->min, node->max,
               node->dur / node->count);
      }

      if (args->show_percentage) {
        const char* label = (parent_inclusive_dur == -1)
                                ? (args->focus_function ? "of self" : "of total")
                                : "of parent";
        printf(" [%.2f%% %s]", percentage, label);
      }

      printf("\n");
    } else if (args->output_format == FORMAT_DOT) {
      char sanitized_name[1024];
      sanitize_for_graphing(node->name, sanitized_name, sizeof(sanitized_name));

      // Calculate the exclusive duration's percentage of the total run time.
      double exclusive_percentage_of_total = 0.0;
      if (total_run_time > 0) {
        exclusive_percentage_of_total = ((double)node->exclusive_dur / total_run_time) * 100.0;
      }

      // Define the node with a label showing its name, exclusive duration, and percentage of total.
      printf("  \"%s\" [label=\"%s\\nExclusive: %lld (%.2f%%)\"];\n", sanitized_name, node->name,
             node->exclusive_dur, exclusive_percentage_of_total);

      // If it's a child node, draw an edge from its parent.
      if (parent_inclusive_dur != -1) {
        char sanitized_parent[1024];
        sanitize_for_graphing(stack_prefix, sanitized_parent, sizeof(sanitized_parent));
        printf("  \"%s\" -> \"%s\";\n", sanitized_parent, sanitized_name);
      }
    }

    if (node->children_count > 0) {
      char new_prefix[256] = "";
      char new_stack_prefix[1024] = "";

      if (args->output_format == FORMAT_TEXT) {
        snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix,
                 (i == count - 1) ? "    " : "│   ");
      }

      // For DOT format, the new stack prefix is the current node's name, used to identify the
      // parent in the recursive call.
      if (args->output_format == FORMAT_DOT) {
        snprintf(new_stack_prefix, sizeof(new_stack_prefix), "%s", node->name);
      }

      print_tree(node->children, node->children_count, args, total_run_time, new_prefix, node->dur,
                 new_stack_prefix);
    }
  }
}

int main(int argc, char* argv[]) {
  // --- Argument Parsing ---
  Args args = {0, 0, 0, 0.0, 0.0, FORMAT_TEXT, NULL, NULL, 0, 20};
  int opt;
  struct option long_options[] = {{"help", no_argument, 0, 'h'},
                                  {"show-percentage", no_argument, 0, 'p'},
                                  {"aggregate", no_argument, 0, 'a'},
                                  {"time-metric", required_argument, 0, 't'},
                                  {"focus-function", required_argument, 0, 'f'},
                                  {"force-sort", no_argument, 0, 's'},
                                  {"output-format", required_argument, 0, 'o'},
                                  {"min-percent-root", required_argument, 0, 256},
                                  {"min-percent-children", required_argument, 0, 257},
                                  {"depth_cut", required_argument, 0, 'd'},
                                  {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "hpat:f:so:d:", long_options, NULL)) != -1) {
    switch (opt) {
      case 'h':
        print_help(argv[0]);
        return 0;
      case 'p':
        args.show_percentage = 1;
        break;
      case 'a':
        args.aggregate = 1;
        break;
      case 't':
        args.is_exclusive_metric = (strcmp(optarg, "exclusive") == 0);
        break;
      case 'f':
        args.focus_function = optarg;
        break;
      case 's':
        args.force_sort = 1;
        break;
      case 'o':
        if (strcmp(optarg, "dot") == 0) {
          args.output_format = FORMAT_DOT;
        }
        break;
      case 'd':
        args.depth_cut = atoi(optarg);
        break;
      case 256:
        args.min_percent_root = atof(optarg);
        break;
      case 257:
        args.min_percent_children = atof(optarg);
        break;
      default:
        print_help(argv[0]);
        return EXIT_FAILURE;
    }
  }
  if (optind >= argc) {
    fprintf(stderr, "Error: Filepath is required.\n\n");
    print_help(argv[0]);
    exit(EXIT_FAILURE);
  }
  args.filepath = argv[optind];

  // --- File Reading ---
  FILE* fp = fopen(args.filepath, "rb");
  if (!fp) {
    perror("Error opening file for analysis");
    return EXIT_FAILURE;
  }
  fseek(fp, 0, SEEK_END);
  long file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  char* buffer = (char*)malloc(file_size + 1);
  if (!buffer || fread(buffer, 1, file_size, fp) != file_size) {
    perror("Error reading file");
    free(buffer);
    fclose(fp);
    return EXIT_FAILURE;
  }
  buffer[file_size] = '\0';
  fclose(fp);

  // --- JSON Parsing and Node Creation ---
  Node** all_nodes = NULL;
  int total_nodes = 0;
  int capacity = 0;
  long long excluded_time = 0;
  char* current_pos = buffer;
  char* end = buffer + file_size;

  while (current_pos < end) {
    char* next_newline = (char*)memchr(current_pos, '\n', end - current_pos);
    char* line_end = (next_newline != NULL) ? next_newline : end;

    if (line_end > current_pos && *(line_end - 1) == '\r') {
      line_end--;
    }

    char original_char = *line_end;
    *line_end = '\0';

    if (strlen(current_pos) > 1) {
      json_object* json = json_tokener_parse(current_pos);
      if (json) {
        json_object* ph_obj;
        if (json_object_object_get_ex(json, "ph", &ph_obj) &&
            json_object_is_type(ph_obj, json_type_string) &&
            strcmp(json_object_get_string(ph_obj), "X") == 0) {
          json_object *name_obj, *ts_obj, *dur_obj;
          if (json_object_object_get_ex(json, "name", &name_obj) &&
              json_object_is_type(name_obj, json_type_string) &&
              json_object_object_get_ex(json, "ts", &ts_obj) &&
              json_object_get_type(ts_obj) != json_type_null &&
              json_object_object_get_ex(json, "dur", &dur_obj) &&
              json_object_get_type(dur_obj) != json_type_null) {
            Node* node = (Node*)calloc(1, sizeof(Node));

            long long dur_val = json_object_get_int64(dur_obj);
            node->name = strdup(json_object_get_string(name_obj));
            node->ts = json_object_get_int64(ts_obj);
            node->dur = dur_val;
            node->ts_end = node->ts + node->dur;
            node->count = 1;
            node->min = dur_val;
            node->max = dur_val;

            if (total_nodes >= capacity) {
              capacity = (capacity == 0) ? 1024 : capacity * 2;
              all_nodes = (Node**)realloc(all_nodes, capacity * sizeof(Node*));
            }
            all_nodes[total_nodes++] = node;
          }
        }
        json_object_put(json);
      }
    }

    *line_end = original_char;
    current_pos = (next_newline != NULL) ? (next_newline + 1) : end;
  }
  free(buffer);

  if (total_nodes == 0) {
    printf("No function call events ('ph': 'X') found in the data.\n");
    if (all_nodes) free(all_nodes);
    return 0;
  }

  // --- Call Tree Construction ---
  if (args.force_sort) {
    qsort(all_nodes, total_nodes, sizeof(Node*), compare_nodes);
  }

  Node** stack = (Node**)malloc(total_nodes * sizeof(Node*));
  int stack_top = -1;
  Node** root_calls = NULL;
  int root_count = 0;
  std::ofstream outFile("depth_cut.txt");

  for (int i = 0; i < total_nodes; i++) {
    Node* call = all_nodes[i];
    bool found = false;

    while (stack_top > -1 && call->ts >= stack[stack_top]->ts_end) {
      stack_top--;
    }

    if (stack_top > -1) {
      Node* parent = stack[stack_top];
      call->depth = parent->depth + 1;

      if (call->depth > args.depth_cut) {
        excluded_time += call->dur;
        if (outFile.is_open()) {
          outFile << call->name << " " << call->dur << std::endl;
        }
        continue;
      }

      if (args.aggregate) {
        for (int j = 0; j < parent->children_count; j++) {
          if (strcmp(parent->children[j]->name, call->name) == 0) {
            if (parent->children[j]->max < call->dur) parent->children[j]->max = call->dur;
            if (parent->children[j]->min > call->dur) parent->children[j]->min = call->dur;
            parent->children[j]->dur += call->dur;
            parent->children[j]->count++;
            found = true;
            break;
          }
        }
      }

      if (!found) {
        if (parent->children_count >= parent->children_capacity) {
          parent->children_capacity =
              (parent->children_capacity == 0) ? 4 : parent->children_capacity * 2;
          parent->children =
              (Node**)realloc(parent->children, parent->children_capacity * sizeof(Node*));
        }
        parent->children[parent->children_count++] = call;
      }
    } else {  // Is a root call
      call->depth = 0;
      if (args.aggregate) {
        for (int j = 0; j < root_count; j++) {
          if (strcmp(root_calls[j]->name, call->name) == 0) {
            if (root_calls[j]->max < call->dur) root_calls[j]->max = call->dur;
            if (root_calls[j]->min > call->dur) root_calls[j]->min = call->dur;
            root_calls[j]->dur += call->dur;
            root_calls[j]->count++;
            found = true;
            break;
          }
        }
      }
      if (!found) {
        root_calls = (Node**)realloc(root_calls, (root_count + 1) * sizeof(Node*));
        root_calls[root_count++] = call;
      }
    }

    stack[++stack_top] = call;
  }
  free(stack);
  outFile.close();

  // --- Data Processing and Output ---
  calculate_exclusive_times(root_calls, root_count);

  if (args.output_format == FORMAT_DOT) {
    printf("digraph G {\n");
  }

  if (args.focus_function) {
    Node** focused_nodes = NULL;
    int focused_count = 0;
    find_nodes_by_name(root_calls, root_count, args.focus_function, &focused_nodes, &focused_count);

    if (focused_count == 0) {
      fprintf(stderr, "\nError: Function '%s' not found in the trace.\n", args.focus_function);
    } else {
      if (args.output_format == FORMAT_TEXT) {
        printf("Found %d instance(s) of '%s'.\n\n", focused_count, args.focus_function);
      }

      for (int i = 0; i < focused_count; i++) {
        if (args.output_format == FORMAT_TEXT) {
          printf("============================================================\n");
          printf("Call graph for '%s' (Instance #%d)\n", args.focus_function, i + 1);
          printf("Total time for this instance: %lld\n", focused_nodes[i]->dur);
          printf("============================================================\n");
        }

        print_tree(&focused_nodes[i], 1, &args, focused_nodes[i]->dur, "", -1, "");
        printf("============================================================\n");
        if (args.output_format == FORMAT_TEXT) printf("\n");
      }
      free(focused_nodes);
    }
  } else {
    if (total_nodes > 0) {
      long long min_ts = -1, max_ts_end = 0;
      if (total_nodes > 0) {  // Ensure all_nodes is not empty
        min_ts = all_nodes[0]->ts;
        for (int i = 0; i < total_nodes; i++) {
          if (all_nodes[i]->ts < min_ts) min_ts = all_nodes[i]->ts;
          if (all_nodes[i]->ts_end > max_ts_end) max_ts_end = all_nodes[i]->ts_end;
        }
      }
      long long total_run_time = (min_ts != -1) ? (max_ts_end - min_ts) : 0;

      if (args.output_format == FORMAT_TEXT) {
        printf("============================================================\n");
        printf(" Call Graph\n");
        printf("Total Trace Duration: %lld\n", total_run_time);
        printf("Excluded Time (due to depth cut): %lld(%f%% of total time)\n", excluded_time,
               (total_run_time > 0) ? (excluded_time * 100.0 / total_run_time) : 0.0);
        printf("============================================================\n");
      }
      print_tree(root_calls, root_count, &args, total_run_time, "", -1, "");
    }
  }

  if (args.output_format == FORMAT_DOT) {
    printf("}\n");
  }

  // --- Final Cleanup ---
  for (int i = 0; i < total_nodes; i++) {
    free(all_nodes[i]->name);
    free(all_nodes[i]->children);
    free(all_nodes[i]);
  }
  free(all_nodes);
  free(root_calls);

  if (args.output_format == FORMAT_TEXT) {
    printf("\ncompleted.\n");
  }
  return 0;
}
