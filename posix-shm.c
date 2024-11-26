#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>

// New type definitions
typedef struct {
    long timestamp;
    char buff[256];
} PC_DATA;

/*--- Shared data ---*/
PC_DATA *shared;

int main(int argc, char* argv[]) {
    // Check the command name (argv[0]) if this program runs
    // as either a writer or a reader
    bool i_am_a_writer = strstr(argv[0], "writer");
    // Create shared memory
    // To avoid name conflict, append my UID as suffix for SHM name
    char mem_name[50];
    sprintf(mem_name, "pc-%d", getuid());
    int mem_desc; // shared memory descriptor

    // Create shared mem in /dev/shm
    mem_desc = shm_open(mem_name,
                  O_CREAT | O_RDWR,   // Create for R/W operations
                  S_IRUSR | S_IWUSR); // Give me R/W permission
    ftruncate(mem_desc, sizeof(PC_DATA));   // Set the shared memory size

    // Map (or "attach") the shared memory to an address in my process
    shared = mmap(NULL, sizeof(PC_DATA), 
            PROT_READ | PROT_WRITE,     // Enable Read/Write access
            MAP_SHARED,                 // Share among processes
            mem_desc, 0);

    if (i_am_a_writer) {
        printf("Enter some text: ");
        fflush(stdout);
        fgets(shared->buff, 256, stdin);
        shared->timestamp = time(NULL);
        munmap(shared, sizeof(PC_DATA));    // unmap from my pointer
    }
    else {
        // Nope, I'm running as a reader
        printf("On %s\tYou typed: %s\n", 
               ctime(&shared->timestamp), shared->buff);
        munmap(shared, sizeof(PC_DATA));    // unmap from my pointer
        // Cleanup
        shm_unlink(mem_name);               // remove from /dev/shm
    }
    return 0;
}
