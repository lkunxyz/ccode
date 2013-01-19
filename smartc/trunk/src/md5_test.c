#include <stdio.h>
#include <stdlib.h>

#include "md5.h"

struct file_md5_t {
	char *md5;
	struct file_md5_t *next;
};

static int congestion = 0;

static void file_list_insert(struct file_md5_t **list, char *md5);
static void file_list_insert(struct file_md5_t **list, char *md5)
{
	struct file_md5_t *h = *list;
	for(; h; h = h->next) {
		if(h->md5 == NULL)
			continue;
		if(!strcmp(h->md5, md5)) {
			printf("Warning! Congestion md5, %s\n", md5);
			congestion++;
			return ;
		}
	}
	
	struct file_md5_t *f = calloc(1, sizeof(*f));
	
	f->md5 = md5;

	f->next = *list;
	*list = f;
}


int main(int argc, char **argv) 
{
	if(argc < 2) {
		fprintf(stderr, "usage: md5_test urllist\n");
		abort();
	}

	char line[4096];
	int count = 0;
	FILE *url_file = fopen(argv[1], "r");
	if(url_file == NULL) 
		exit(1);
	
	struct file_md5_t *filelist = calloc(1, sizeof(*filelist));
	while(fgets(line, 4096, url_file)) {
		char *md5 = strdup(md5_string_get(line, strlen(line)));
		
		file_list_insert(&filelist, md5);
		printf("%d\n", count++);
	}
	
	fclose(url_file);
	printf("congestion %d\n", congestion);
	return 0;
}
