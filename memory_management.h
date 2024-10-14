#ifndef MEMORY_MANAGEMENT_H
#define MEMORY_MANAGEMENT_H

#define MAX_LEN 8
#define MAX_MEMORY 2048  // Total memory size in KB
#define QUANTUM 1  // Quantum time in seconds
#define PAGE_SIZE 4  // 4KB per page
#define NUM_FRAMES (MAX_MEMORY / PAGE_SIZE)  // frame number
#define MAX_FRAMES_PER_PROCESS 2048
#define MIN_PAGES 4 // the minimum pages that needed in virtual memory allocation algorithm

typedef enum { READY, RUNNING, FINISHED } State;

typedef struct node {
    char pid[MAX_LEN + 1];
    int arr_time;
    int remain_time;
    int memory;  // Memory requirement in KB
    int addr;  // Starting address of the allocated memory
    int page_to_frame_mapping[MAX_FRAMES_PER_PROCESS];
    int num_pages;  // Total pages required by the process
    State state;
    int assigned_frames[MAX_FRAMES_PER_PROCESS];  // Array to hold frame indices
    int frames_count;  // Number of frames allocated
    int required_pages;  // pages that the process need
    struct node *next;
    int isValid;
} node_t;

int memory[MAX_MEMORY] = {0};  // Memory blocks

typedef struct {
    node_t *head;
    node_t *foot;
} list_t;

typedef struct {
    int in_use;  // if the frame is in use，0 for free，1 for in use
    int last_used;  // the last used time based on LRU
    char pid[MAX_LEN + 1];  // The ID of the process that occupies the frame, used for tracing and debugging
} Frame;

Frame frames[NUM_FRAMES];  // the total frame number

typedef struct EvictResult {
    int* evicted_frames;
    int num_evicted;
    int success;
} EvictResult;

void initialize_frames();
list_t* make_empty_list(void);
void insert_at_foot(list_t* list, node_t* new_node);
node_t* remove_from_front(list_t* list);
node_t* create_node(char* pid, int arr_time, int remain_time, int memory);
void read_input(list_t* input_queue, char* filename);
int first_fit(node_t *node);
void deallocate(node_t *node);
int calculate_memory_usage_first_fit(node_t *node);
int evict_page_paged(int current_time);
int* find_least_used_frames(int frames_to_evict);
EvictResult allocate_pages(node_t* process, int current_time);
EvictResult allocate_virtual_pages(node_t* process, int current_time);
void round_robin_scheduler(list_t* ready_queue, int quantum, char *strategy);
void free_list(list_t *list);
int calculate_memory_usage();
int find_free_frame();
char* format_frames_list(node_t *node, int count);
int ready_queue_length(list_t *list);
int isValidNode(node_t* node);

#endif // MEMORY_MANAGEMENT_H
