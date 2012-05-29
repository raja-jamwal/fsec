/*
	Fsec - Find bad Sectors on disk
        Copyright (C) 2012  Raja Jamwal - linux1@zoho.com

 		This program is free software; you can redistribute it and/or modify
 		it under the terms of the GNU General Public License as published by
 		the Free Software Foundation; either version 2 of the License, or
 	        (at your option) any later version.

	       This program is distributed in the hope that it will be useful,
               but WITHOUT ANY WARRANTY; without even the implied warranty of
               MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
               GNU General Public License for more details.

               You should have received a copy of the GNU General Public License
               along with this program; if not, write to the Free Software
               Foundation, Inc., 59 Temple Place - Suite 330,
               Boston, MA 02111-1307, USA.

	Compilation instructions:
	
		gcc main.c -o fsec

	Program parameters:
		
		./fsec --block-size <bs> --skip <sp>

		<bs> : Block size, number of bytes to read in one go, this also determines our block unit
		       Bad sectors will be displayed in block unit, i.e 3 block or 5000 block

		<sp> : Skip number of block (not bytes), this is done when a bad block is encountered,
		       Disk may or may not seek further once a bad block is encountered,
		       In such a scenario <sp> should be atleast 1 or any relavant value 
	
	NOTE : status shown is <current byte>/<total byte>, the values may or may not be wrong,
	       depends on the condition of failing disk

	In any case, the values shown in block units are final 

*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

struct options
{
	char * device_file;
	unsigned int block_size;
	unsigned int skip;
	int64_t start_block;	// TODO
};


int 
init_options(struct options * op, int argv, char ** argc)
{
	struct options * pt = op;

	/*initialize data in structure*/

	int i;
	for (i=0; i<argv; i++)
	{
	
		if (strcmp(argc[i],"--device")==0)
		{
			if (argc[i+1])
			{
			int size = strlen(argc[i+1]);
			pt->device_file = (char*) malloc(size+1);
			strcpy(pt->device_file, argc[i+1]);
			}
		}

		if (strcmp(argc[i],"--block-size")==0)
		{
			if (argc[i+1])
			{
			pt->block_size = atoi(argc[i+1]);
			}
		}
	
		if (strcmp(argc[i],"--skip")==0)
		{
			if (argc[i+1])
			{
			pt->skip = atoi(argc[i+1]);
			}
		}

	}

	fprintf(stderr, "Device:\n %s, block_size %i, skip %i blocks\n", pt->device_file, pt->block_size, pt->skip);

}

char * dummy = NULL;
void 
progress(int64_t current, int64_t total)
{
	char * line = (char*) malloc (512);
	sprintf(line, "%llu/%llu %lld%%", current, total, (current*100ULL)/total);

	if (dummy != NULL)
	{
		int i;
		for(i=0; i<strlen(dummy); i++)
		{
			printf("%c", 0x08);
		}
		fflush(stdout);
		free(dummy);
	}

	printf("%s", line);
	fflush(stdout);

	dummy = (char*) malloc(strlen(line)+1);
	strcpy (dummy, line);	
	free(line);
}

void 
copy_notice()
{
 printf("\n Fsec version aplha, Copyright (C) 2012 Raja Jamwal\n\
 Fsec comes with ABSOLUTELY NO WARRANTY;\n\
 This is free software, and you are welcome to redistribute it\n\
 under certain conditions;\n\n");
}

int
main(int argv, char ** argc)
{
	copy_notice();

	struct options * args = calloc (1, sizeof(struct options));

	init_options (args, argv, argc);

	int64_t sector = 0;	

	int fid = open(args->device_file, O_RDONLY);

	if (fid<0)
	{
		printf("Cannot open device, error\n");
		return 1;
	}

	int result = 1;
	char * sec = (char*) malloc ((args->block_size)+1);
	unsigned int errors = 0;

	int64_t total = (int64_t) lseek64 (fid, 0ULL, SEEK_END);
	lseek64 (fid, 0ULL, SEEK_SET);

	int64_t previous = 0ULL;

	int seeking = FALSE;

	while (result != 0)
	{
		result = read(fid, sec, args->block_size);

		seeking = TRUE;
		
		if(result == -1)
		{
			 errors++;

			 printf(" %lld block: %s\n", sector, strerror(errno));
			 
			 if (args->skip > 0)
			 {
				printf(" skipping %i blocks\n", args->skip);
				
				if (lseek64 (fid, (int64_t) (args->skip)*(args->block_size), SEEK_CUR) == -1)
				{
					printf(" not seeking: %s\n", strerror(errno));
					seeking == FALSE;
				}
				else
				{
					sector += (int64_t) (args->skip);
				}
			 } 
			 
		}

		//progress ( (int64_t) lseek64 (fid, (int64_t) 0, SEEK_CUR), total);
		int64_t current = (int64_t) lseek64 (fid, 0ULL, SEEK_CUR);
	
		if ((current - previous) > 10000000ULL)
		{
			progress((int64_t) lseek64 (fid, 0ULL, SEEK_CUR), total);
			previous = current;
		}

		if (seeking == TRUE)
		{
			sector++;
		}
	}

	progress((int64_t) lseek64 (fid, 0ULL, SEEK_CUR), total);
	free(sec);
	
	if (!close(fid)){
		printf("Unable to close device, error\n");
		return 1;
	}

	printf("Device closed\n");
	return 0;
}
