//Deondre North
//CWID: 11871481
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct PageTable {
    int table[256];
} PageTable;

typedef struct Page {
    char page[256];
} Page;

typedef struct PhysicalMemory {
    Page *memory;
    int *inUse;
    int *lru;
    int size;
} PhysicalMemory;

typedef struct TLB{
    int pages[16];
    int frames[16];
    int next;
} TLB;

int getPageNumber(int address) {
    int num = address >> 8;
    return num;
}

int getOffset(int address) {
    int mask = (1 << 8) - 1;
    int num = address & mask;
    return num;
}

int getEmptyFrame(PhysicalMemory memory) {
    for (int i = 0; i < memory.size; i++) {
        if (memory.inUse[i] == -1)
            return i;
    }

    int oldest = 0;
    int age = memory.lru[0];

    for (int i = 0; i < memory.size; i++) {
        if (memory.lru[i] < age) {
            oldest = i;
            age = memory.lru[i];
        }
    }


    return oldest;
}

int searchTLB(int page, TLB tlb){
    for(int i = 0; i < 16; i++){
        if(tlb.pages[i] == page){
            return tlb.frames[i];
        }
    }
    return -1;
}

void replaceTLB(int page, int frame, TLB *tlb){
    for(int i = 0; i < 16; i++){
        if(tlb->pages[i] == -1){
            tlb->pages[i] = page;
            tlb->frames[i] = frame;
            return;
        }
    }

    tlb->pages[tlb->next] = page;
    tlb->frames[tlb->next] = frame;

    tlb->next++;
    if(tlb->next >= 16)
        tlb->next = 0;

    return;
}

int main(int argc, char* argv[])
{
    if(argc < 2){
        printf("You need to provide the name of the address file as a command line argument\n");
        return 1;
    }
    int frameSize = 256;
    if(argc == 3)
        frameSize = atoi(argv[2]);

    FILE* file = fopen(argv[1], "r");
    FILE* store = fopen("BACKING_STORE.bin", "rb");
    unsigned int addresses[1000];
    int count = 0;
    PageTable table;
    PhysicalMemory memory;
    TLB tlb;
    tlb.next = 0;
    int tlbHits = 0, tlbMisses = 0, pageFaults = 0;

    //init memory
    memory.memory = malloc(sizeof(Page)*frameSize);
    memory.inUse = malloc(sizeof(int)*frameSize);
    memory.lru = malloc(sizeof(int)*frameSize);
    memory.size = frameSize;
    for (int i = 0; i < 256; i++) {
        table.table[i] = -1;
    }

    for(int i = 0; i < frameSize; i++){
        memory.inUse[i] = -1;
        memory.lru[i] = 0;
    }

    for(int i = 0; i < 16; i++){
        tlb.pages[i] = -1;
    }


    for (int i = 0; i < 1000; i++) {
        fscanf(file, "%d", &addresses[i]);
    }

    for (int i = 0; i < 1000; i++) {
        int address = addresses[i];
        int mask = (1 << 16) - 1;
        address = address & mask;
        int page = getPageNumber(address);
        int offset = getOffset(address);

        int frame = searchTLB(page, tlb);
        if(frame == -1){
            tlbMisses++;
            frame = table.table[page];
            if (frame == -1) {
                pageFaults++;
                frame = getEmptyFrame(memory);
                table.table[page] = frame;
                memory.inUse[frame] = 1;
                memory.lru[frame]++;
                fseek(store, page*256, SEEK_SET);

                fread(memory.memory[frame].page, sizeof(Page), 1, store);
            }
            replaceTLB(page, frame, &tlb);
        }else{
            tlbHits++;
        }
        int physical = (frame << 8) | offset;
        printf("Virtual address: %d Physical address: %d Value: %d\n", address, physical, memory.memory[frame].page[offset]);
    }


    printf("Number of Translated Addresses = 1000\n");
    printf("Page Faults = %d\n", pageFaults);
    printf("Page Fault Rate = %.3f\n", pageFaults/1000.0);
    printf("TLB Hits = %d\n", tlbHits);
    printf("TLB Hit Rate = %.3f\n", tlbHits/1000.0);

    free (memory.memory);
    free (memory.inUse);
    free (memory.lru);

    return 0;
}
