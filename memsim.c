#include <stdio.h>
#include <string.h>
#include<stdlib.h>

typedef struct {
        int pageNo;
        int modified;
		int t;
		// int lastUsed;  // Used for LRU
   		int useBit;    // Used for Clock
} page;
enum repl { random1, fifo, lru, clocks};
int createMMU(int);
int checkInMemory(int) ;
int allocateFrame(int) ;
page selectVictim(int, enum repl) ;
const int pageoffset = 12;            /* Page size is fixed to 4 KB */
int numFrames ;

page *pageTable; // Array of pages
// int *frameTable; // Array of frames
int t=0;
// int currentTime = 0; // For LRU and Clock tracking
int clockHand = 0; // For Clock algorithm
int disk_writes = 0; // Global variable to track disk writes

/* Creates the page table structure to record memory allocation */
int createMMU (int frames)
{
	numFrames = frames;
    pageTable = (page *)malloc(sizeof(page) * numFrames);
    // frameTable = (int *)malloc(sizeof(int) * numFrames);
    for (int i = 0; i < numFrames; i++) {
        pageTable[i].pageNo = -1; // Initialize to empty (-1 means no page)
        pageTable[i].modified = 0;
        // pageTable[i].lastUsed = 0;
		pageTable[i].t = -1;
        pageTable[i].useBit = 0;
        // frameTable[i] = -1; // Initialize to empty
    }

        // to do

        return 0;
}

int checkInMemory( int page_number){

    int result = -1;
	for(int i=0;i<numFrames;i++){
		if(pageTable[i].pageNo == page_number){
			pageTable[i].useBit =1;
			pageTable[i].t = t++;
			result = i;
			break;
		}
	}
        return result ;
}
/* Handles write operations */
void handleWriteOperation(int frame_no) {
    if (frame_no != -1) {
        // Mark the page in the page table as modified if it's a write operation
        pageTable[frame_no].modified = 1;
    }
}

/* allocate page to the next free frame and record where it put it */
int allocateFrame(int page_number)
{
    for (int i=0; i<numFrames; i++) {
        if (pageTable[i].pageNo == -1) {
			pageTable[i].t = t++;
			pageTable[i].useBit = 1; // For Clock algorithm
            pageTable[i].pageNo = page_number;
            pageTable[i].modified = 0; // Not modified yet
            // pageTable[i].lastUsed = currentTime++; // Set the last used time (for LRU)
            return i; // Return frame number (index in pageTable)
        }
    }
    return -1; // No free frames (should never happen here)
}


page selectVictim(int page_number,enum repl mode) {
    page victim;// Variable to hold the victim page
    int i = -1;

    switch(mode ) {
		  case random1: {
			// Randomly select a frame 
            i = rand() % numFrames;  
            break;
        }
        case fifo:
        case lru: {
			// For FIFO and LRU policies, select the page with the earliest time
            int earlyTime = t;
            for(int j=0;j<numFrames;j++) {
                if(pageTable[j].t< earlyTime) {
                    earlyTime= pageTable[j].t;
                    i= j; // Update the index of the victim page
                }
            }
            break;
        }
        case clocks: {
			 // Find a page with its reference bit set to 0
            while (pageTable[clockHand].useBit != 0) {
                pageTable[clockHand].useBit = 0; 
                clockHand= (clockHand + 1)% numFrames; // Move to next frame 
            }
            i = clockHand;// Select the page at the current clock hand position
            clockHand= (clockHand + 1) % numFrames;// Move the clock hand to the next position
            break;
        }
        default:
            break;
    }
	// If a valid index was found, replace the page and return the victim
    if(i != -1) {
        victim= pageTable[i];
        pageTable[i]= (page){.pageNo = page_number, .modified = 0, .t = t++};// Replace the page
        return victim;
    }
	// Return an invalid page if no valid victim was selected
    return (page){.pageNo = -1, .modified = 0};
}


int main(int argc, char *argv[])
{
  
	char *tracename;
	int	page_number,frame_no, done ;
	int	do_line, i;
	int	no_events, disk_writes, disk_reads;
	int debugmode;
 	enum repl  replace;
	int	allocated=0; 
	int	victim_page;
        unsigned address;
    	char rw;
	page Pvictim;
	FILE *trace;


        if (argc < 5) {
             printf("Usage: ./memsim inputfile numberframes replacementmode debugmode \n");
             exit ( -1);
	}
	else {
        tracename= argv[1];	
	trace = fopen( tracename, "r");
	if (trace== NULL ) {
             printf("Cannot open trace file %s \n", tracename);
             exit (-1);
	}
	numFrames= atoi(argv[2]);
        if (numFrames< 1) {
            printf("Frame number must be at least 1\n");
            exit (-1);
        }
        if (strcmp(argv[3],"lru\0") == 0)
            replace = lru;
	    else if (strcmp(argv[3], "rand\0") == 0)
	     replace = random1;
	          else if (strcmp(argv[3], "clock\0") == 0)
                       replace = clocks;		 
	               else if (strcmp(argv[3], "fifo\0") == 0)
                             replace = fifo;		 
        else 
	  {
             printf("Replacement algorithm must be rand/fifo/lru/clock  \n");
             exit (-1);
	  }

        if (strcmp(argv[4],"quiet\0") == 0)
            debugmode = 0;
	else if (strcmp(argv[4],"debug\0") == 0)
            debugmode = 1;
        else 
	  {
             printf("Replacement algorithm must be quiet/debug  \n");
             exit (-1);
	  }
	}
	
	done = createMMU (numFrames);
	if ( done == -1 ) {
		 printf( "Cannot create MMU" ) ;
		 exit(-1);
        }
	no_events= 0 ;
	disk_writes= 0 ;
	disk_reads= 0 ;

        do_line= fscanf(trace,"%x %c",&address,&rw);
	while ( do_line == 2)
	{
		page_number=  address >> pageoffset;
		frame_no= checkInMemory( page_number) ;    /* ask for physical address */


		if (frame_no == -1 )
		{
		  disk_reads++ ;			/* Page fault, need to load it into memory */
		  if (debugmode) 
		      printf( "Page fault %8d \n", page_number) ;
		  if (allocated < numFrames)  			/* allocate it to an empty frame */
		   {
                     frame_no = allocateFrame(page_number);
		     allocated++;
                   }
                   else{
		      Pvictim = selectVictim(page_number, replace) ;   /* returns page number of the victim  */
		      frame_no = checkInMemory( page_number) ;    /* find out the frame the new page is in */
		   if (Pvictim.modified)           /* need to know victim page and modified  */
	 	      {
                      disk_writes++;			    
                      if (debugmode) printf("Disk write %8d \n", Pvictim.pageNo) ;
		      }
		   else
                      if (debugmode) printf("Discard %8d \n", Pvictim.pageNo) ;
		   }
		}
		if ( rw == 'R'){
		    if (debugmode) printf( "reading %8d \n", page_number) ;
		}
		else if ( rw == 'W'){
		    // mark page in page table as written - modified  
			handleWriteOperation(frame_no);
		    if (debugmode) printf( "writing %8d \n", page_number) ;
		}
		 else {
		      printf( "Badly formatted file. Error on line %d\n", no_events+1); 
		      exit (-1);
		}

		no_events++;
        	do_line = fscanf(trace,"%x %c",&address,&rw);
	}

	printf( "total memory frames: %d\n", numFrames);
	printf( "events in trace: %d\n", no_events);
	printf( "total disk reads: %d\n", disk_reads);
	printf( "total disk writes: %d\n", disk_writes);
	printf( "page fault rate: %.4f\n", (float) disk_reads/no_events);
}
				
