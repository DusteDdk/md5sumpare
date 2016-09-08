#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

int fsize(char *filename) {
    struct stat st; 

    if (stat(filename, &st) == 0)
        return st.st_size;

    return -1; 
}

int main(int argc, char** argv) {
	FILE *fp;
	char *fn;
	char* data;
	int size=0;
	int dataread=0;
	int numbuckets[65536];
	if( argc!= 2 ) {
		puts("Required argument: FILE\n");
		return(1);
	}
	fn=argv[1];
	printf("Attempting to open %s...\n", fn);
	size = fsize(fn);

	if( size < 0 ) { 
		puts("Could not get file size.\n");
		return(1);
	}

	printf("File is %i bytes...\n", size);
	

	fp=fopen(fn, "r");
	if(!fp) {
		puts("Could not open the file..\n");
		return(1);
	}

	data = malloc(size);

	if(!data) {
		puts("Could not allocate memory for the file...\n");
		return(1);
	}

	dataread = fread(data, size, 1, fp);

	if(dataread != 1) {
		puts("Could not read data..\n");
		return(1);
	}

	printf("Read %i bytes.\n", size);

	int numLines=0;
	int c=0;
	
	for(c=0; c < size; c++) {
		if(data[c]=='\n')
		{
			numLines++;
		}
	}
	
	printf("There are %i linebreaks.\n", numLines);

	struct breaks {
		int pos;
		int buckidx;
		char duped;
	};
	
	struct breaks *linebreaks;
	linebreaks = malloc( sizeof(struct breaks)*numLines );
	memset(linebreaks, 0, sizeof(struct breaks)*numLines);

	int lastbreak=0;
	int idx=0;
	int cc=0;
	char lift;
	memset(numbuckets, 0, sizeof(int)*(0xffff+1) );
	for(c=0; c<size; c++) {
		if(data[c]=='\n') {
			//Found a line
			linebreaks[idx].pos=lastbreak;
			//add count to bucket
			lift = data[lastbreak+4];
			data[lastbreak+4]=0;
			linebreaks[idx].buckidx=(int)strtol(data+lastbreak, NULL, 16);
			numbuckets[linebreaks[idx].buckidx ]++;
			data[lastbreak+4]=lift;
			lastbreak=c+1;
			data[c]=0;
			idx++;
		}
	}

	struct breaks **bbuck[0xffff+1];
	
	int usedbuckets=0;
	int usedplaces=0;
	for(c=0; c< 0xffff+1; c++) {
		if( numbuckets[c] ) {
			usedbuckets++;
			bbuck[c] = malloc( sizeof(struct breaks*)*numbuckets[c] );
			usedplaces += numbuckets[c];
		}
	}

	printf("Allocated %i places in %i buckets..\n",usedplaces, usedbuckets);

	memset( numbuckets,0, sizeof(int)*(0xffff+1) );

	for(c=0; c < numLines; c++) {
		cc = linebreaks[c].buckidx;
		idx = numbuckets[cc];
		numbuckets[cc]++;
		bbuck[cc][idx] = &linebreaks[c];
//		printf("Assigning bucket %i place %i to line %i\n", cc, idx, c);
	}
	
	int linepos;
	char* str;
	puts("Comparing...");
	int numduped=0;
	int numcomps=0;

	for(c=0; c< 0xffff+1; c++) {
		if( numbuckets[c] ) {
			//printf("Bucket %i should have %i elements...\n", c, numbuckets[c]);
			for(idx=0; idx < numbuckets[c]; idx++ ) {
				linepos=bbuck[c][idx]->pos;
				str=data+linepos;
				//printf("Bucket %i, pos %i has linepos %i (%s)\n", c, idx, linepos, str);
				if( bbuck[c][idx]->duped == 0 ) {
					for(cc=idx+1; cc < numbuckets[c]; cc++) {
						numcomps++;
						if( memcmp( str, data+bbuck[c][cc]->pos, 32 ) == 0 ) {
							if(bbuck[c][idx]->duped==0) {
								bbuck[c][idx]->duped=1;
								puts("-------------- Identical Files Found ----------------");
								puts( bbuck[c][idx]->pos + data );
								numduped++;
							}
							puts( bbuck[c][cc]->pos + data );
							bbuck[c][cc]->duped=1;
							numduped++;
						}
					}
				}
			}
		}
	}
	puts("------------------- Comparison Done  ----------------");
	printf("Did %i comparisons.\nFound %i identical.\n", numcomps, numduped);

	for(c=0; c< 0xffff+1; c++) {
		if(numbuckets[c]) {
			free(bbuck[c]);
		}
	}
	free(linebreaks);
	free(data);
	fclose(fp);
	return(0);
}
