
#include <stdlib.h>
#include <stdio.h>
#include "stack.h"
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

#define _XOPEN_SOURCE 500
#include <ftw.h>

#define ANSI_COLOR_RED     "\x1b[91m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_MAGNETA "\x1b[35m"
#define ANSI_COLOR_RESET   "\x1b[0m"

struct stat compare_file;
char oper[1];
char path_link[PATH_MAX];
int link_flag;

void print_priv(const struct stat *st)
{
	if (st->st_mode & S_IRUSR) printf(ANSI_COLOR_GREEN "r"); else printf(ANSI_COLOR_RESET "-");
	if (st->st_mode & S_IWUSR) printf(ANSI_COLOR_BLUE "w"); else printf(ANSI_COLOR_RESET "-");
	if (st->st_mode & S_IXUSR) printf(ANSI_COLOR_YELLOW "x"); else printf(ANSI_COLOR_RESET "-");
	if (st->st_mode & S_IRGRP) printf(ANSI_COLOR_GREEN "r"); else printf(ANSI_COLOR_RESET "-");
	if (st->st_mode & S_IWGRP) printf(ANSI_COLOR_BLUE "w"); else printf(ANSI_COLOR_RESET "-");
	if (st->st_mode & S_IXGRP) printf(ANSI_COLOR_YELLOW "x"); else printf(ANSI_COLOR_RESET "-");
	if (st->st_mode & S_IROTH) printf(ANSI_COLOR_GREEN "r"); else printf(ANSI_COLOR_RESET "-");
	if (st->st_mode & S_IWOTH) printf(ANSI_COLOR_BLUE "w"); else printf(ANSI_COLOR_RESET "-");
	if (st->st_mode & S_IXOTH) printf(ANSI_COLOR_YELLOW "x"); else printf(ANSI_COLOR_RESET "-");
	printf(ANSI_COLOR_RESET "");
}

int compare_dates(time_t time1, time_t time2)
{
	int y1, m1, d1, y2, m2, d2;
	struct tm* date;
	date = localtime(&time1);
	y1 = date->tm_year; m1 = date->tm_mon; d1 = date->tm_mday;
	date = localtime(&time2);
	y2 = date->tm_year; m2 = date->tm_mon; d2 = date->tm_mday;
	if (y1 > y2) return 1;
	if (y1 < y2) return -1;
	if (m1 > m2) return 1;
	if (m1 < m2) return -1;
	if (d1 > d2) return 1;
	if (d1 < d2) return -1;
	return 0;
}

int date_predicate(time_t date1, time_t date2, const char* operator)
{
	int res = compare_dates(date1, date2);
	if (strcmp(operator, "<") == 0) return (res < 0);
	else if (strcmp(operator, "=") == 0) return (res == 0);
	return (res > 0);
}

int fn(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
	if (typeflag == FTW_F)
	{
		if (! date_predicate(sb->st_mtime, compare_file.st_mtime, oper)) return 0;

		if (link_flag < 2 && strcmp(fpath, path_link) != 0)
	    {
	        strcpy(path_link, fpath);
	        strcat(path_link, "_linkfromnftw");
	    	link(fpath, path_link);
	    	link_flag++;
	    }

		print_priv(sb);
		printf(" %ld\t", sb->st_size);
		char mod_date[100];
	    strftime(mod_date, 1000, " %b %d %H:%M", localtime(&sb->st_mtime));
	    printf(ANSI_COLOR_MAGNETA "%s" ANSI_COLOR_RESET, mod_date);
	    printf(" %s\n", fpath);
	}
	return 0;
}

int main(int argc, char const *argv[])
{

//////////////////////////////////////////////////////////

//			VALIDATING ARGUMENTS

//////////////////////////////////////////////////////////

	if (argc < 4 || (strcmp(argv[2],"<")!=0 && strcmp(argv[2],"=")!=0 && strcmp(argv[2],">")!=0))
	{
		printf("Incorrect arguments!\n");
		return 1;
	}
	strcpy(oper, argv[2]);

	char arg_file[PATH_MAX];
	if (! realpath(strdup(argv[1]), arg_file))
	{
		perror(argv[1]);
		return 1;
	}

	if (! stat(argv[3], &compare_file) == 0)
	{
		perror(argv[3]);
		return 1;
	}

	link_flag = 0;
	
//////////////////////////////////////////////////////////

//			USING OPENDIR

//////////////////////////////////////////////////////////

	printf(ANSI_COLOR_RED "Using readdir:\n" ANSI_COLOR_RESET);

	char path[PATH_MAX];
	char path_new[PATH_MAX];
	char mod_date[100];
	int openv;
	DIR* dirp;
	struct dirent* file;
	struct stat st;

	stack_s* dirs = init_stack();
	push(dirs, arg_file, strlen(arg_file));
	while (! is_empty(dirs))
	{
		pop(dirs, path);
		openv = 1;
		if((dirp = opendir(path)) )
		{
	        while((file = readdir(dirp))) 
	        {	
	        	if (strcmp(file->d_name, "..")==0 || strcmp(file->d_name, ".")==0 || 
	        		(file->d_type!=DT_DIR && file->d_type!=DT_REG)) continue;
	        	
	        	strcpy(path_new, path);
	        	strcat(path_new, "/");
	        	strcat(path_new, strdup(file->d_name));
	        	if (file->d_type == DT_DIR)
	        	{
	        		push(dirs, path_new, strlen(path_new));
	        		continue;
	        	}

	        	stat(path_new, &st);
	        	if (! date_predicate(st.st_mtime, compare_file.st_mtime, oper)) continue;

	        	if (link_flag < 1)
	        	{
	        		strcpy(path_link, path_new);
	        		strcat(path_link, "_linkfromopendir");
	        		link(path_new, path_link);
	        		link_flag++;
	        	}

	        	if (openv && file->d_type == DT_REG)
	        	{
	        		printf(ANSI_COLOR_CYAN "\n%s:\n" ANSI_COLOR_RESET, path);
	        		openv--;
	        	}

	        	print_priv(&st);

	        	printf(" %ld\t", st.st_size);
	        	strftime(mod_date, 1000, " %b %d %H:%M", localtime(&st.st_mtime));
	        	printf(ANSI_COLOR_MAGNETA "%s" ANSI_COLOR_RESET, mod_date);
	        	printf(" %s\n", path_new);
	        }
	        closedir(dirp);
    	}
    	else
    	{
    		if (errno == EACCES) printf("Permission to %s denied.", path);
    		perror(path);
    		if (errno == ENOTDIR)
    		{
    			delete_stack(dirs);
    			return 1;
    		}
    	}
	}
	delete_stack(dirs);
	
//////////////////////////////////////////////////////////

//			USING NFTW

//////////////////////////////////////////////////////////

	printf(ANSI_COLOR_RED "\n\nUsing nftw:\n\n" ANSI_COLOR_RESET);

	if (nftw(arg_file, &fn, 10, NULL) < 0)
	{
		perror("NFTW error");
		return 1;
	}

	return 0;
}