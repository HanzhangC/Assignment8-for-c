
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include "memory_management.h"

void initialize_frames() {
    for (int i = 0; i < NUM_FRAMES; i++) {
        frames[i].in_use = 0;
        frames[i].last_used = 0;
    }
}

list_t* make_empty_list(void) {
    list_t* list = malloc(sizeof(list_t));
    assert(list);

    list->head = NULL;
    list->foot = NULL;
    return list;
}

void insert_at_foot(list_t* list, node_t* new_node) {
    if (list->head == NULL) {
        list->head = new_node;
        list->foot = new_node;
    } else {
        list->foot->next = new_node;
        list->foot = new_node;
    }
    new_node->next = NULL;
}

node_t* remove_from_front(list_t* list){
if (list == NULL || list->head == NULL) {
        return NULL;
    }

    node_t *temp = list->head;
    list->head = list->head->next;

    if (list->head == NULL) {
        list->foot = NULL;
    }

    temp->next = NULL;
    return temp;
}

node_t* create_node(char* pid, int arr_time, int remain_time, int memory) {
    node_t* new_node = malloc(sizeof(node_t));
    if (!new_node) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    strcpy(new_node->pid, pid);
    new_node->arr_time = arr_time;
    new_node->remain_time = remain_time;
    new_node->memory = memory;
    new_node->addr = -1;  // Memory not yet allocated
    new_node->state = READY;
    new_node->next = NULL;
    new_node->isValid = 1;
    return new_node;
}

int isValidNode(node_t* node) {
    return node != NULL && node->isValid;
}

void read_input(list_t* input_queue, char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open file\n");
        exit(1);
    }

    int arr_time, remain_time, memory;
    char pid[MAX_LEN + 1];

    while (fscanf(fp, "%d %s %d %d", &arr_time, pid, &remain_time, &memory) == 4) {
        node_t* new_node = create_node(pid, arr_time, remain_time, memory);
        insert_at_foot(input_queue, new_node);
    }

    fclose(fp);
}

int first_fit(node_t *node) {
    if (node->memory <= 0) return -1;
    for (int i = 0; i < MAX_MEMORY; i++) {
        int free = 1;
        for (int j = i; j < i + node->memory; j++) {
            if (memory[j] != 0) {
                free = 0;
                break;
            }
        }
        if (free) {
            node->addr = i;
            for (int k = i; k < i + node->memory; k++) {
                memory[k] = 1;
            }
            return i;
        }
    }
    printf("Failed to allocate size %d, procee: %s\n", node->memory, node->pid);
    return -1;
}

void deallocate(node_t *node) {
    // Check if the address is valid before trying to deallocate
    if (node->addr < 0 || node->addr + node->memory > MAX_MEMORY) {
        return; // Return immediately without attempting to deallocate
    }
    
    // Proceed with deallocation if the address is valid
    for (int i = node->addr; i <  node->addr + node->memory; i++) {
        memory[i] = 0;
    }
}

int calculate_memory_usage_first_fit(node_t *node){
    int used_memory = 0;
    for (int i = 0; i < MAX_MEMORY; i++) {
        if (memory[i]) {
            used_memory++;
        }
    }
    return (int)((double)used_memory / MAX_MEMORY * 100);
}

void free_list(list_t *list) {
    if (list != NULL) {
        node_t *current = list->head;
        node_t *next;
        while (current != NULL) {
            next = current->next;
            free(current);
            current = next;
        }
        free(list);
    }
}

int ready_queue_length(list_t *list) {
    if (!list) {
        fprintf(stderr, "Error: Passed a NULL list to ready_queue_length.\n");
        return 0; // If the list pointer is NULL, return 0
    }
    int count = 0;
    node_t *current = list->head;
    while (current != NULL) {
        count++;
        if (current->next && !isValidNode(current->next)) {
            fprintf(stderr, "Error: Detected a corrupted or freed node in the list.\n");
            break; // Break the loop if the next node is detected as invalid
        }
        current = current->next;
    }
    return count;
}

int calculate_memory_usage() {
    int used_frames = 0;
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (frames[i].in_use) {
            used_frames++;
        }
    }
    return (int)((double)used_frames / NUM_FRAMES * 100);
}

char* format_frames_list(node_t *node, int count) {
    int estimated_size = count * 6 + 2; // Slightly larger to accommodate commas and spaces
    char* buffer = malloc(estimated_size);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    char* ptr = buffer;
    int first = 1; 
    ptr += sprintf(ptr, "[");
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (strcmp(frames[i].pid, node->pid) == 0) { // 检查帧是否属于指定的进程
            if (!first) {
                ptr += sprintf(ptr, ", ");
            } else {
                first = 0; // 首个元素后设置为0，后续的前面都要加逗号
            }
            ptr += sprintf(ptr, "%d", i);
        }
    }
    sprintf(ptr, "]");
    return buffer;
}

int evict_page_paged(int current_time) {
    int lru_index = -1;
    int oldest_time = INT_MAX;
    
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (frames[i].in_use && frames[i].last_used < oldest_time) {
            oldest_time = frames[i].last_used;
            lru_index = i;
        }
    }

    if (lru_index != -1) {
        frames[lru_index].in_use = 0;
        frames[lru_index].last_used = current_time;  // Reset time since it's being evicted
        return lru_index;
    }

    printf("No frames available to evict\n");
    return -1;
}

EvictResult allocate_pages(node_t* process, int current_time) {
    EvictResult result;
    result.evicted_frames = calloc(NUM_FRAMES, sizeof(int)); // remember to free after use
    result.num_evicted = 0;
    result.success = 0;

    int needed_pages = (process->memory + PAGE_SIZE - 1) / PAGE_SIZE;
    if (needed_pages > NUM_FRAMES) {
        printf("Process %s requires more pages (%d) than available frames (%d).\n", process->pid, needed_pages, NUM_FRAMES);
        return result;
    }
    
    int allocated_pages = 0;

    for (int i = 0; i < NUM_FRAMES && allocated_pages < needed_pages; i++) {
        if (!frames[i].in_use) {  // Check if the frame is free
            frames[i].in_use = 1;  // Mark the frame as used
            frames[i].last_used = current_time;  // Update the last used time
            process->assigned_frames[allocated_pages] = i;  // Store the frame number
            if (allocated_pages == 0) { // Update addr to point to the first allocated frame
                process->addr = i;
            }
            allocated_pages++;
        }
    }

    // If more frames are needed, try to evict used frames
    while (allocated_pages < needed_pages) {
        int evicted_frame = evict_page_paged(current_time);  // Evict a page
        if (evicted_frame != -1) {
            frames[evicted_frame].in_use = 1;  // Mark the frame as used
            frames[evicted_frame].last_used = current_time;  // Update the last used time
            process->assigned_frames[allocated_pages] = evicted_frame;  // Store the frame number
            

            if (allocated_pages == 0){
                process->addr = evicted_frame;
            }
            
            allocated_pages++;
            result.evicted_frames[result.num_evicted++] = evicted_frame;
        } else {
            printf("Failed to evict any frame, all frames are in use.\n");
            // Rollback any frames already marked as in use if we cannot meet the needed pages
            for (int i = 0; i < allocated_pages; i++) {
                frames[process->assigned_frames[i]].in_use = 0;
            }
            return result; // Return failure if not enough frames can be allocated
        }
    }

    process->frames_count = allocated_pages;
    result.success = allocated_pages == needed_pages;  // Return success if all needed pages are successfully allocated
    return result;
}


// virtual memory allocation implement

int* find_least_used_frames(int num_frames_to_evict) {
    int* least_used_frames = malloc(num_frames_to_evict * sizeof(int));
    if (!least_used_frames) return NULL;

    // Initialize the array with -1.
    for (int i = 0; i < num_frames_to_evict; i++) {
        least_used_frames[i] = -1;
    }

    // We will track the number of frames placed into the least_used_frames array.
    int frames_placed = 0;

    // Iterate over all frames to find the least used ones.
    while (frames_placed < num_frames_to_evict) {
        int frame_to_place = -1;
        int oldest_use = INT_MAX; // Use the maximum int value as the initial oldest use.

        // Find the least recently used frame that is not already in the least_used_frames array.
        for (int i = 0; i < NUM_FRAMES; i++) {
            if (frames[i].in_use) {
                int is_already_chosen = 0;
                for (int j = 0; j < frames_placed; j++) {
                    if (least_used_frames[j] == i) {
                        is_already_chosen = 1;
                        break;
                    }
                }
                if (!is_already_chosen && frames[i].last_used <= oldest_use) {
                    // If the last_used is equal, prefer the smaller frame index.
                    if (frames[i].last_used < oldest_use || frame_to_place == -1) {
                        oldest_use = frames[i].last_used;
                        frame_to_place = i;
                    }
                }
            }
        }

        // If we found a frame to place, add it to the array.
        if (frame_to_place != -1) {
            least_used_frames[frames_placed] = frame_to_place;
            frames_placed++;
        } else {
            // No more frames can be evicted; we break out of the loop.
            break;
        }
    }

    return least_used_frames; // Returns an array containing the indexes of the least used frames
}

void free_frame(int frame_number) {
    if (frame_number >= 0 && frame_number < NUM_FRAMES) {
        frames[frame_number].in_use = 0;  // marked as not in use
        frames[frame_number].last_used = 0;  // reset the last used time
    }
}

void print_and_free_evicted_frames(int time, int *evicted_frames, int num_evicted) {
    if (num_evicted > 0) {
        printf("%d,EVICTED,evicted-frames=[", time);
        for (int i = 0; i < num_evicted; i++) {
            printf("%d%s", evicted_frames[i], i < num_evicted - 1 ? "," : "");
        }
        printf("]\n");
        free(evicted_frames);
    }
}

EvictResult allocate_virtual_pages(node_t* process, int current_time) {
    EvictResult result;
    result.evicted_frames = calloc(NUM_FRAMES, sizeof(int));
    result.num_evicted = 0;
    result.success = 0;

    int needed_pages = (process->memory + PAGE_SIZE - 1) / PAGE_SIZE;
    if (needed_pages > NUM_FRAMES) {
        printf("Process %s requires more pages (%d) than available frames (%d).\n", process->pid, needed_pages, NUM_FRAMES);
        return result;
    }

    int min_required_pages = needed_pages < MIN_PAGES ? needed_pages : MIN_PAGES;
    int allocated_pages = 0;

    for (int i = 0; i < NUM_FRAMES && allocated_pages < needed_pages; i++){
       if (frames[i].in_use && strcmp(frames[i].pid, process->pid) == 0) {
            frames[i].last_used = current_time; // Simply update last used time if already allocated to this process
            process->assigned_frames[allocated_pages] = i;
            allocated_pages++;
            continue; // Continue to the next iteration without modifying in_use or PID
        }

        if (!frames[i].in_use){
            frames[i].in_use = 1; // the frame is now in use
            frames[i].last_used = current_time; // update the time of the frame being used.
            strncpy(frames[i].pid, process->pid, MAX_LEN);
            frames[i].pid[MAX_LEN] = '\0'; // 确保有 null-terminator
            process->assigned_frames[allocated_pages] = i;
            if (allocated_pages == 0){
                process->addr = i;
            }
            allocated_pages ++;
        }
    }

    if (allocated_pages < min_required_pages) {
        int num_frames_to_evict = min_required_pages - allocated_pages;
        int* least_used_frames = find_least_used_frames(num_frames_to_evict);

        for (int i = 0; least_used_frames && i < num_frames_to_evict; i++) { // make sure the least_used_frames is not NULL
            int frame_to_evict = least_used_frames[i];
            if (frame_to_evict != -1 && frames[frame_to_evict].in_use) {
                // found one frame to evict and free it
                free_frame(frame_to_evict);

                // realloce the evicted frame to the process
                frames[frame_to_evict].in_use = 1;
                frames[frame_to_evict].last_used = current_time;
                strncpy(frames[i].pid, process->pid, MAX_LEN);
                frames[i].pid[MAX_LEN] = '\0'; 
                process->assigned_frames[allocated_pages] = frame_to_evict;

                if (allocated_pages == 0) {
                    process->addr = frame_to_evict;  // if this is the first allocation, then set the address of the process.
                }

                allocated_pages++;
                result.evicted_frames[result.num_evicted++] = frame_to_evict;
            }
        }
        print_and_free_evicted_frames(current_time, result.evicted_frames, result.num_evicted);
        free(least_used_frames);  // free the memory of least_used_frames.
    }
    
    process->frames_count = allocated_pages;
    result.success = allocated_pages >= min_required_pages;
    
    return result;
}

void round_robin_scheduler(list_t* input_queue, int quantum, char *strategy) {

    int time;
    node_t *current = NULL, *prev = NULL;
    list_t *ready_queue = make_empty_list();

    if (input_queue->head == NULL){
        printf("There is not any process to be excuted for now.\n");
        return;
    }

    time = input_queue->head->arr_time;
    
    while (ready_queue->head != NULL || input_queue->head != NULL) {


        // Move processes whose arrival time has come to the ready queue
        while (input_queue->head != NULL && input_queue->head->arr_time <= time + quantum) {
            node_t* process_ready = remove_from_front(input_queue);
            insert_at_foot(ready_queue, process_ready);
            }

        if (ready_queue->head == NULL && input_queue->head != NULL) {
            int arr_time = input_queue->head->arr_time;

            if (arr_time % quantum == 0) {
                time = arr_time;
            } else {
                time = arr_time + (quantum - arr_time % quantum);
            }
            continue;
        }

        if (ready_queue->head == NULL) {
            break;
        }

        // Memory management based on strategy
        current = ready_queue->head;
        current->required_pages = (current->memory + PAGE_SIZE - 1) / PAGE_SIZE;
        int allocated = 0;
        EvictResult result;

        if (strcmp(strategy, "infinite") != 0) {
            if (strcmp(strategy, "first_fit") == 0) {
                if (current->addr == -1) {  // If memory not yet allocated
                    current->addr = first_fit(current);
                }
                allocated = 1;
            } else if (strcmp(strategy, "paged") == 0) {
                if (current->frames_count == 0) {  // If pages not yet allocated
                result = allocate_pages(current, time);
                allocated = result.success;
                print_and_free_evicted_frames(time, result.evicted_frames, result.num_evicted);
                result.evicted_frames = NULL;
                } else {
                    allocated = 1;
                }
            } else if (strcmp(strategy, "virtual") == 0) {
                result = allocate_virtual_pages(current,time);
                allocated = result.success;
            }

            int failure_count = 0;
            int length =  ready_queue_length(ready_queue);
       
            if (current->addr == -1 || !allocated) {  // Check if memory allocation was successful
                failure_count++;

               // if fail to allocate memory, put the process at the tail of the queue
                if (failure_count > length) {
                    printf("All processes are stuck due to memory allocation failures.\n");
                    break;
                }
                if (current->next != NULL) {
                    ready_queue->head = current->next;
                    insert_at_foot(ready_queue, current);
                }
                continue;  // Skip this cycle as the process cannot run
            }
            else {
                failure_count = 0; // reset as the allocation successes
            }
        }

        // Process can now run
        current->state = RUNNING;
        int mem_usage = calculate_memory_usage(); // Calculate memory usage
        while(current->pid != prev->pid){
            if (strcmp(strategy, "infinite") == 0) {
                printf("%d,RUNNING,process-name=%s,remaining-time=%d\n",
                    time, current->pid, current->remain_time);
            } else if (strcmp(strategy, "first_fit") == 0) {
                int memory_use = calculate_memory_usage_first_fit(current);
                printf("%d,RUNNING,process-name=%s,remaining-time=%d,mem-usage=%d%%,allocated-at=%d\n",
                    time, current->pid, current->remain_time, memory_use, current->addr);
            } else if (strcmp(strategy, "paged") == 0 || strcmp(strategy, "virtual") == 0) {
                char * buffer = format_frames_list(current, current->frames_count);
                printf("%d,RUNNING,process-name=%s,remaining-time=%d,mem-usage=%d%%,mem-frames=%s\n",
                    time, current->pid, current->remain_time, mem_usage, buffer);
                    free (buffer);
            }
            prev = current;
        }
        int actual_quantum = (current->remain_time > quantum) ? quantum : current->remain_time;
        current->remain_time -= actual_quantum;
        time += quantum;  // Increment time by the quantum used

        if (current->remain_time > 0) {
            if (!current->next) {
                continue;  // Keep running if it's the only process
            }
            // Move to end
            ready_queue->head = current->next;
            insert_at_foot(ready_queue, current);
        }
        else if (current->remain_time < quantum){
            // Process completes
            int length = ready_queue_length(ready_queue);
            printf("%d,FINISHED,process-name=%s,proc-remaining=%d\n", time, current->pid, length - 1 );
            ready_queue->head = current->next;
            if (strcmp(strategy, "first_fit") == 0 || strcmp(strategy, "paged") == 0 || strcmp(strategy, "virtual") == 0) {
                deallocate(current);  // Free the allocated memory if not using infinite memory
            }
            free(current);  // Free the node
        }
    }
}



int main(int argc, char **argv) {

    // if the input parameters correct
    if (argc != 7) {
        printf("Invalid input!\n");
        return 0;
    }

    char* filename = NULL;
    int quantum = 0;
    char* strategy = NULL;

    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-f") == 0) {
            filename = argv[i + 1];
        } else if (strcmp(argv[i], "-q") == 0) {
            quantum = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-m") == 0) {
            strategy = argv[i + 1];
        }
    }

    if (filename == NULL || strategy == NULL || quantum <= 0) {
        fprintf(stderr, "Invalid arguments\n");
        return 1;
    }

    list_t *input_queue = make_empty_list();
    read_input(input_queue, filename);
    
    if (strcmp(strategy, "infinite") == 0) {
        round_robin_scheduler(input_queue, quantum, "infinite");
    } else if (strcmp(strategy, "first_fit") == 0) {
        round_robin_scheduler(input_queue, quantum, "first_fit");
    } else if (strcmp(strategy, "paged") == 0) {
        round_robin_scheduler(input_queue, quantum, "paged");
    } else if (strcmp(strategy, "virtual") == 0) {
        round_robin_scheduler(input_queue, quantum, "virtual");
    }
    else {
        fprintf(stderr, "Unsupported memory strategy\n");
        return 1;
    }

    free_list(input_queue);;
    return 0;
}

