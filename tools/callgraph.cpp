#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
#include <ctype.h>
#include "cJSON.h"

typedef enum {
    FORMAT_TEXT,
    FORMAT_DOT
} OutputFormat;

typedef struct Node {
    char* name;
    long long ts;
    long long dur;
    long long ts_end;
    long long exclusive_dur;
    struct Node** children;
    int children_count;
    int children_capacity;
    //int line_num; // Diagnostic: original line number
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
} Args;

void print_help(const char* prog_name) {
    printf("Usage: %s <filepath> [options]\n\n", prog_name);
    printf("Analyzes a performance trace file to build a call graph.\n\n");
    printf("Options:\n");
    printf("  -h, --help                     Show this help message and exit.\n");
    printf("  -p, --show-percentage          Display the percentage of time each function took (text mode only).\n");
    printf("  -t, --time-metric <type>       Metric for time display (text mode only). <type> can be 'inclusive' or 'exclusive'.\n");
    printf("  -f, --focus-function <name>    Focus the output on all instances of a specific function.\n");
    printf("  -s, --force-sort               Force the program to sort the trace data if it might be out of order.\n");
    printf("  -o, --output-format <format>   Specify the output format. <format> can be:\n");
    printf("                                   'text' (default): Human-readable call tree.\n");
    printf("                                   'dot': DOT language file for Graphviz.\n");
    printf("      --min-percent-root <num>   Hide root functions that are less than <num>%% of the total trace time.\n");
    printf("      --min-percent-children <num> Hide child functions that are less than <num>%% of their parent's time.\n");
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

void find_nodes_by_name(Node** nodes, int count, const char* name, Node*** found_nodes, int* found_count) {
    for (int i = 0; i < count; i++) {
        if (strcmp(nodes[i]->name, name) == 0) {
            *found_nodes = (Node**)realloc(*found_nodes, (*found_count + 1) * sizeof(Node*));
            (*found_nodes)[*found_count] = nodes[i];
            (*found_count)++;
        }
        if (nodes[i]->children_count > 0) {
            find_nodes_by_name(nodes[i]->children, nodes[i]->children_count, name, found_nodes, found_count);
        }
    }
}

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

void print_tree(Node** nodes, int count, Args* args, long long total_run_time, const char* prefix, long long parent_inclusive_dur, const char* stack_prefix) {
    for (int i = 0; i < count; i++) {
        Node* node = nodes[i];
        
        double percentage = 0.0;
        if (parent_inclusive_dur == -1) { // Root node
            if (total_run_time > 0) percentage = ((double)node->dur / total_run_time) * 100.0;
            if (args->min_percent_root > 0 && percentage < args->min_percent_root) {
                continue;
            }
        } else { // Child node
            if (parent_inclusive_dur > 0) percentage = ((double)node->dur / parent_inclusive_dur) * 100.0;
            if (args->min_percent_children > 0 && percentage < args->min_percent_children) {
                continue;
            }
        }
        
        if (args->output_format == FORMAT_TEXT) {
            const char* connector = (i == count - 1) ? "└── " : "├── ";
            printf("%s%s%s (ts: %lld, dur: %lld [%s])", prefix, connector, node->name, node->ts, 
                   args->is_exclusive_metric ? node->exclusive_dur : node->dur,
                   args->is_exclusive_metric ? "exclusive" : "inclusive");
                   //node->line_num);

            if (args->show_percentage) {
                const char* label = (parent_inclusive_dur == -1) ? (args->focus_function ? "of self" : "of total") : "of parent";
                printf(" [%.2f%% %s]", percentage, label);
            }
            printf("\n");
        } else if (args->output_format == FORMAT_DOT) {
            if (parent_inclusive_dur != -1) {
                char sanitized_parent[1024];
                char sanitized_name[1024];
                sanitize_for_graphing(stack_prefix, sanitized_parent, sizeof(sanitized_parent));
                sanitize_for_graphing(node->name, sanitized_name, sizeof(sanitized_name));
                printf("  \"%s\" -> \"%s\";\n", sanitized_parent, sanitized_name);
            }
        }

        if (node->children_count > 0) {
            char new_prefix[256] = "";
            char new_stack_prefix[1024] = "";

            if (args->output_format == FORMAT_TEXT) {
                snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, (i == count - 1) ? "    " : "│   ");
            }
            
            if (args->output_format == FORMAT_DOT) {
                snprintf(new_stack_prefix, sizeof(new_stack_prefix), "%s", node->name);
            }

            print_tree(node->children, node->children_count, args, total_run_time, new_prefix, node->dur, new_stack_prefix);
        }
    }
}

// --- Core Logic ---

int main(int argc, char* argv[]) {
    // --- Argument Parsing ---
    Args args = {0, 0, 0, 0.0, 0.0, FORMAT_TEXT, NULL, NULL};
    int opt;
    struct option long_options[] = {
        {"help",               no_argument,       0, 'h'},
        {"show-percentage",    no_argument,       0, 'p'},
        {"time-metric",        required_argument, 0, 't'},
        {"focus-function",     required_argument, 0, 'f'},
        {"force-sort",         no_argument,       0, 's'},
        {"min-percent-root",   required_argument, 0, 256},
        {"min-percent-children", required_argument, 0, 257},
        {"output-format",      required_argument, 0, 'o'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "hpt:f:so:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h': print_help(argv[0]); return 0;
            case 'p': args.show_percentage = 1; break;
            case 't': args.is_exclusive_metric = (strcmp(optarg, "exclusive") == 0); break;
            case 'f': args.focus_function = optarg; break;
            case 's': args.force_sort = 1; break;
            case 'o':
                if (strcmp(optarg, "dot") == 0) {
                    args.output_format = FORMAT_DOT;
                }
                break;
            case 256: args.min_percent_root = atof(optarg); break;
            case 257: args.min_percent_children = atof(optarg); break;
            default: print_help(argv[0]); return EXIT_FAILURE;
        }
    }
    if (optind >= argc) {
        fprintf(stderr, "Error: Filepath is required.\n\n");
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }
    args.filepath = argv[optind];

    // --- Main Analysis Mode ---
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

    Node** all_nodes = NULL;
    int total_nodes = 0;
    int capacity = 0;
    int line_counter = 0;

    char* current_pos = buffer;
    char* end = buffer + file_size;

    while (current_pos < end) {
        line_counter++;
        char* next_newline = (char*)memchr(current_pos, '\n', end - current_pos);
        
        char* line_end;
        if (next_newline != NULL) {
            line_end = next_newline;
        } else {
            line_end = end;
        }

        if (line_end > current_pos && *(line_end - 1) == '\r') {
            line_end--;
        }

        char original_char = *line_end;
        *line_end = '\0';
        
        if (strlen(current_pos) > 1) {
            cJSON* json = cJSON_Parse(current_pos);
            if (json) {
                cJSON* ph = cJSON_GetObjectItem(json, "ph");
                if (ph && cJSON_IsString(ph) && strcmp(ph->valuestring, "X") == 0) {
                    Node* node = (Node*)calloc(1, sizeof(Node));
                    cJSON* name = cJSON_GetObjectItem(json, "name");
                    cJSON* ts = cJSON_GetObjectItem(json, "ts");
                    cJSON* dur = cJSON_GetObjectItem(json, "dur");
                    
                    if (name && cJSON_IsString(name) && ts && cJSON_IsNumber(ts) && dur && cJSON_IsNumber(dur)) {
                        node->name = strdup(name->valuestring);
                        node->ts = (long long)ts->valuedouble;
                        node->dur = (long long)dur->valuedouble;
                        node->ts_end = node->ts + node->dur;
                        //node->line_num = line_counter;

                        if (total_nodes >= capacity) {
                            capacity = (capacity == 0) ? 1024 : capacity * 2;
                            all_nodes = (Node**)realloc(all_nodes, capacity * sizeof(Node*));
                        }
                        all_nodes[total_nodes++] = node;
                    } else {
                        free(node);
                    }
                }
                cJSON_Delete(json);
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

    if (args.force_sort) {
        qsort(all_nodes, total_nodes, sizeof(Node*), compare_nodes);
    }

    Node** stack = (Node**)malloc(total_nodes * sizeof(Node*));
    int stack_top = -1;
    Node** root_calls = NULL;
    int root_count = 0;

    for (int i = 0; i < total_nodes; i++) {
        Node* call = all_nodes[i];
        while (stack_top > -1 && call->ts_end >= stack[stack_top]->ts_end) {
            stack_top--;
        }
        if (stack_top > -1) {
            Node* parent = stack[stack_top];
            if (parent->children_count >= parent->children_capacity) {
                parent->children_capacity = (parent->children_capacity == 0) ? 4 : parent->children_capacity * 2;
                parent->children = (Node**)realloc(parent->children, parent->children_capacity * sizeof(Node*));
            }
            parent->children[parent->children_count++] = call;
        } else {
            root_calls = (Node**)realloc(root_calls, (root_count + 1) * sizeof(Node*));
            root_calls[root_count++] = call;
        }
        stack[++stack_top] = call;
    }
    free(stack);

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
                if (args.output_format == FORMAT_TEXT) printf("\n");
            }
            free(focused_nodes);
        }
    } else {
        if (total_nodes > 0) {
            long long min_ts = all_nodes[0]->ts;
            long long max_ts_end = 0;
            for(int i = 0; i < total_nodes; i++) {
                if(all_nodes[i]->ts_end > max_ts_end) max_ts_end = all_nodes[i]->ts_end;
            }
            long long total_run_time = max_ts_end - min_ts;
            
            if (args.output_format == FORMAT_TEXT) {
                printf("============================================================\n");
                printf(" Call Graph\n");
                printf("Total Trace Duration: %lld\n", total_run_time);
                printf("============================================================\n");
            }
            print_tree(root_calls, root_count, &args, total_run_time, "", -1, "");
        }
    }
    
    if (args.output_format == FORMAT_DOT) {
        printf("}\n");
    }

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


// --- Function Implementations ---


