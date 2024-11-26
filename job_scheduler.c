#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROCESSES 100
#define MAX_BURSTS 10
#define MAX_IO 5

typedef enum { NEW, READY, RUNNING, BLOCKED, EXIT } ProcessState;

typedef struct {
    int arrival_time;
    int num_bursts;
    int cpu_bursts[MAX_BURSTS];
    int io_bursts[MAX_IO];
    int current_burst;
    int current_io;
    int remaining_burst;
    int total_burst_time;
    int wait_time;
    int turn_around_time;
    ProcessState state;
} Process;

typedef struct {
    Process processes[MAX_PROCESSES];
    int size;
} RoundRobinQueue;

// function prototypes
void queue_round_robin(RoundRobinQueue *queue, Process process);
Process dequeue_round_robin(RoundRobinQueue *queue);
void parse_input_file(const char *filename, RoundRobinQueue *queue);
void print_process_state(int time, Process *process);
void simulate(RoundRobinQueue *queue, int timeQuantum, int *cpu_time_used);
void print_statistics(RoundRobinQueue *queue, int total_time, int cpu_time_used);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <time_quantum>\n", argv[0]);
        return 1;
    }

    int timeQuantum = atoi(argv[1]);
    RoundRobinQueue rrQueue = { .size = 0 };

    // parses input file
    parse_input_file("input.txt", &rrQueue);

    int cpu_time_used = 0;  // initializes the CPU time used counter

    // runs the simulation
    simulate(&rrQueue, timeQuantum, &cpu_time_used);

    // calculates and prints statistics
    print_statistics(&rrQueue, 100, cpu_time_used);  // example total time

    return 0;
}

// parses the input file to create processes
void parse_input_file(const char *filename, RoundRobinQueue *queue) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error opening file.\n");
        exit(1);
    }

    int arrival_time, num_bursts;
    while (fscanf(file, "%d %d", &arrival_time, &num_bursts) == 2) {
        Process p = { .arrival_time = arrival_time, .num_bursts = num_bursts, .state = NEW, .wait_time = 0, .turn_around_time = 0 };
        p.total_burst_time = 0;
        for (int i = 0; i < num_bursts; i++) {
            fscanf(file, "%d", &p.cpu_bursts[i]);
            p.total_burst_time += p.cpu_bursts[i];
        }
        for (int i = 0; i < num_bursts - 1; i++) {
            fscanf(file, "%d", &p.io_bursts[i]);
        }
        p.current_burst = 0;
        p.current_io = 0;
        p.remaining_burst = p.cpu_bursts[0];
        queue_round_robin(queue, p);
    }

    fclose(file);
}

// prints the current state of the process
void print_process_state(int time, Process *process) {
    const char *state_str[] = { "NEW", "READY", "RUNNING", "BLOCKED", "EXIT" };
    printf("Time %d: Process %d state changed to %s\n", time, process->arrival_time, state_str[process->state]);
}

// queues a process in Round Robin Queue
void queue_round_robin(RoundRobinQueue *queue, Process process) {
    queue->processes[queue->size++] = process;
}

// dequeues a process from round robin queue
Process dequeue_round_robin(RoundRobinQueue *queue) {
    Process process = queue->processes[0];
    for (int i = 0; i < queue->size - 1; i++) {
        queue->processes[i] = queue->processes[i + 1];
    }
    queue->size--;
    return process;
}

// simulates the Round Robin scheduling
void simulate(RoundRobinQueue *queue, int timeQuantum, int *cpu_time_used) {
    int time = 0;
    RoundRobinQueue readyQueue = { .size = 0 };

    while (queue->size > 0 || readyQueue.size > 0) {
        // moves processes to the ready queue if their arrival time is now
        for (int i = 0; i < queue->size; i++) {
            if (queue->processes[i].arrival_time <= time) {
                Process p = dequeue_round_robin(queue);
                p.state = READY;
                print_process_state(time, &p);
                queue_round_robin(&readyQueue, p);
            }
        }

        // if there is a process in the ready queue, process it using round robin
        if (readyQueue.size > 0) {
            Process *current_process = &readyQueue.processes[0];
            current_process->state = RUNNING;
            print_process_state(time, current_process);

            // runs the process for timeQuantum or until its burst is done
            int run_time = (current_process->remaining_burst > timeQuantum) ? timeQuantum : current_process->remaining_burst;
            time += run_time;
            *cpu_time_used += run_time;  // increments the CPU time used by the run time
            current_process->remaining_burst -= run_time;

            if (current_process->remaining_burst == 0) {
                // process has completed CPU burst, move to I/O
                current_process->state = BLOCKED;
                print_process_state(time, current_process);
                if (current_process->current_burst < current_process->num_bursts - 1) {
                    // handles I/O burst
                    current_process->current_burst++;
                    current_process->remaining_burst = current_process->cpu_bursts[current_process->current_burst];
                    current_process->state = READY;
                    queue_round_robin(&readyQueue, *current_process);
                } else {
                    // process has completed all bursts, exit
                    current_process->state = EXIT;
                    print_process_state(time, current_process);
                    // removes from ready queue
                    for (int i = 0; i < readyQueue.size - 1; i++) {
                        readyQueue.processes[i] = readyQueue.processes[i + 1];
                    }
                    readyQueue.size--;
                }
            } else {
                // process has remaining burst time
                queue_round_robin(&readyQueue, *current_process);
            }
        } else {
            // no process to run, simulate idle CPU time
            time++;
        }
    }

    // prints CPU time used when done
    printf("Total CPU Time Used: %d\n", *cpu_time_used);  
}

// prints final statistics 
void print_statistics(RoundRobinQueue *queue, int total_time, int cpu_time_used) {
    int total_turnaround = 0;
    int total_wait_time = 0;

    for (int i = 0; i < queue->size; i++) {
        Process *p = &queue->processes[i];
        p->turn_around_time = total_time - p->arrival_time;
        total_turnaround += p->turn_around_time;
        p->wait_time = p->turn_around_time - p->total_burst_time;
        total_wait_time += p->wait_time;

        printf("Job %d terminated, Turn-Around-Time = %d, Wait time = %d\n", 
               p->arrival_time, p->turn_around_time, p->wait_time);
    }

    double cpu_utilization = (double)cpu_time_used / total_time * 100;  
    printf("CPU Utilization: %.2f%%\n", cpu_utilization);
    printf("Average Turnaround Time: %.2f\n", (double)total_turnaround / queue->size);
    printf("Average Wait Time: %.2f\n", (double)total_wait_time / queue->size);
}
