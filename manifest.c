/* Opal Manifest
 * for Unix
 * version 1
 */


/* pieces section */

#include <errno.h>
/* errno
 */

#include <stdio.h>
/* size_t
 * NULL
 * EOF
 * getc()
 * putc()
 * fputs()
 * printf()
 * fprintf()
 * sprintf()
 * perror()
 */

#include <stdlib.h>
/* size_t
 * NULL
 * EXIT_FAILURE
 * EXIT_SUCCESS
 * exit()
 * malloc()
 * realloc()
 * free()
 */

#include <string.h>
/* strlen()
 * strcpy()
 * strcmp()
 * strerror_l()
 */

#include <stdint.h>
/* uintmax_t
 */

#include <stdbool.h>
/* bool
 * true
 * false
 */

#include <dirent.h>
/* DIR
 * struct dirent
 * opendir()
 * readdir()
 * closedir()
 */

#include <locale.h>
/* uselocale()
 */

#include <unistd.h>
/* getopt()
 */

#include <sys/stat.h>
/* struct stat
 * stat()
 * S_ISREG()
 * S_ISDIR()
 */


/* definitions section */

/* program options */
struct opt_struct {
	bool verbose;

	/* file types */
	bool regular, directory,
		chr_dev, blk_dev,
		symlink, fifo;

	/* update options */
	bool update, add, remove, modified;

	/* metadata types */
	bool size, mtime;

	/* symlink options */
	bool cmd_lnk, /* follow symlinks specified in the command line */
		all_lnk; /* follow all symlinks */
};

/* directory record */
struct dir_rec {
	char *path;
	size_t ino_count;
	ino_t *ino_list;
	dev_t *dev_list;
	struct dir_rec *next;
};

/* file list context */
struct file_list_con {
	bool verbose, follow_link;

	/* files status buffer */
	struct stat *statbuf;

	/* current directory */
	struct dir_rec *c_dir;

	/* last directory */
	struct dir_rec *l_dir;

	/* path prefix length */
	size_t pre_len;

	/* file path space */
	size_t space;

	char *f_path;

	/* currently open directory */
	DIR *dp;
};

/* hierarchy cache */
struct h_cache {

};

/* file node record */
struct node_record {
	char *name;
	size_t count;
	struct node_record *child;
};

/* filename list cache */
struct l_cache {

};

/* manifest record */
struct man_rec {
	struct timespec mtime;
	struct timespec atime;
};


/* functions section */

/* print error message and quit */
void fail(char *message)
{
	/* print error message */
	fputs(message, stderr);
	/* elaborate on the error if possible */
	if(errno) fprintf(stderr, ": %s", strerror_l(errno, uselocale(0)));
	putc('\n', stderr);
	exit(EXIT_FAILURE);
}

/* "failed to" <error message> and quit */
void failed(char *message)
{
	/* prepend "failed to" to the error message */
	fputs("failed to ", stderr);
	fail(message);
}

/* help message */
void help()
{
	char message[] = "Opal Manifest\n"
	"version 1\n\n"

	"This program creates a manifest file containing metadata of files in a file hierarchy. The files are specified on the command line and the manifest file is output to standard output. A manifest file that needs to be updated can be input through standard input.\n\n"

	"options\n"
	"h: output help and exit\n"
	"t: file types to output\n"
	"u: update mode\n"
	"m: types of metadata to include\n"
	"H: archive the files pointed at by symlinks specified in the command line instead of the symlinks themselves\n"
	"L: archive the files pointed at by all symlinks encountered in the hierarchy instead of the symlinks themselves\n\n"

	"The t, u, and m options are followed by characters that specify their behavior.\n\n"

	"file type options\n"
	"r: regular files\n"
	"d: directories\n"
	"c: character devices\n"
	"b: block devices\n"
	"l: symbolic links\n"
	"f: fifos (pipes)\n\n"

	"update options\n"
	"a: add\n"
	"r: remove\n"
	"m: update modified\n\n"

	"metadata type options\n"
	"s: file size\n"
	"m: modification time\n";

	fputs(message, stderr);
}

/* get file status */
int get_stat(bool follow_link, char *fn, struct stat *statbuf, bool verbose)
{
	/* if traversing the filesystem logically */
	if(follow_link)
	{
		/* get logical file status */
		if(stat(fn, statbuf) == -1)
		{
			if(verbose) perror(fn);
			return -1;
		}
		else return 0;
	}

	/* if traversing the filesystem physically */
	else
	{
		/* get physical file status */
		if(lstat(fn, statbuf) == -1)
		{
			if(verbose) perror(fn);
			return -1;
		}
		else return 0;
	}
}

/* read header of an input manifest file */
void read_header(void)
{
	char *line = NULL, *token, delim[] = " \n";
	size_t len = 0;
	int version = 0, c, prev = '\n';

	if(getline(&line, &len, stdin) == -1)
	{
		fputs("invalid input file\n", stderr);
		exit(EXIT_FAILURE);
	}

	token = strtok(line, delim);

	if((token == NULL) || strcmp(token, "OUmanifest"))
	{
		fputs("input is not an Opal manifest file for Unix\n", stderr);
		exit(EXIT_FAILURE);
	}

	token = strtok(NULL, delim);

	if((token == NULL) || (sscanf(token, "%d", &version) != 1))
	{
		fputs("invalid manifest version number\n", stderr);
		exit(EXIT_FAILURE);
	}

	if(version != 1)
	{
		fputs("unsupported manifest version number\n", stderr);
		exit(EXIT_FAILURE);
	}

	while((c = getchar()) != EOF)
	{
		if((c == '\n') && (prev == '\n')) break;
		prev = c;
	}

	free(line);
}

/* add directory record */
void dr_add(struct file_list_con *flc)
{
	size_t i;
	struct dir_rec *c_dir, *l_dir;

	c_dir = flc->c_dir;
	l_dir = flc->l_dir;

	/* check for infinite directory loop */
	for(i = 0; i < c_dir->ino_count; i++)
		if(flc->statbuf->st_ino == c_dir->ino_list[i])
			if(flc->statbuf->st_dev == c_dir->dev_list[i])
				fail("infinite directory loop");

	/* allocate new directory record */
	if((l_dir->next = malloc(sizeof(struct dir_rec))) == NULL) failed("allocate directory record");
	flc->l_dir = l_dir = l_dir->next;

	/* new directory path */
	if((l_dir->path = malloc(strlen(flc->f_path) + 1)) == NULL) failed("allocate directory path");
	strcpy(l_dir->path, flc->f_path);

	/* new inode number list */
	l_dir->ino_count = c_dir->ino_count + 1;
	if((l_dir->ino_list = malloc(l_dir->ino_count * sizeof(ino_t))) == NULL)
		failed("allocate inode number list");
	for(i = 0; i < c_dir->ino_count; i++)
		l_dir->ino_list[i] = c_dir->ino_list[i];
	l_dir->ino_list[c_dir->ino_count] = flc->statbuf->st_ino;

	/* new device number list */
	if((l_dir->dev_list = malloc(l_dir->ino_count * sizeof(dev_t))) == NULL)
		failed("allocate device number list");
	for(i = 0; i < c_dir->ino_count; i++)
		l_dir->dev_list[i] = c_dir->dev_list[i];
	l_dir->dev_list[c_dir->ino_count] = flc->statbuf->st_dev;

	/* terminate linked list */
	l_dir->next = NULL;
}

/* get next directory record */
struct dir_rec * dr_next(struct dir_rec *c_dir)
{
	struct dir_rec *n_dir;

	n_dir = c_dir->next;
	free(c_dir->path);
	free(c_dir->ino_list);
	free(c_dir->dev_list);
	free(c_dir);

	return n_dir;
}

/* prepare file list */
struct file_list_con * fl_prep(char *root, struct stat *statbuf, struct opt_struct *opts)
{
	struct file_list_con *flc;
	struct dir_rec *f_dir;

	/* allocate new file list context */
	if((flc = malloc(sizeof(struct file_list_con))) == NULL) failed("allocate file list context");

	flc->verbose = opts->verbose;
	flc->follow_link = opts->all_lnk;
	flc->statbuf = statbuf;
	flc->space = 0;
	flc->f_path = NULL;

	/* allocate first directory record */
	if((f_dir = malloc(sizeof(struct dir_rec))) == NULL) failed("allocate first directory record");
	flc->l_dir = flc->c_dir = f_dir;

	if(root != NULL)
	{
		flc->pre_len = strlen(root);

		/* store directory name */
		if((f_dir->path = malloc(flc->pre_len + 1)) == NULL) failed("allocate first directory path");
		strcpy(f_dir->path, root);

		/* open directory */
		if((flc->dp = opendir(root)) == NULL)
		{
			perror(f_dir->path);
			free(f_dir->path);
			free(f_dir);
			free(flc);
			return NULL;
		}
	}
	else
	{
		f_dir->path = NULL;
		flc->pre_len = 0;

		/* open directory */
		if((flc->dp = opendir(".")) == NULL)
		{
			perror(f_dir->path);
			free(f_dir->path);
			free(f_dir);
			free(flc);
			return NULL;
		}
	}

	f_dir->ino_count = 1;

	/* create first inode number list */
	if((f_dir->ino_list = malloc(sizeof(ino_t))) == NULL)
		failed("allocate first inode number list");
	f_dir->ino_list[0] = statbuf->st_ino;

	/* create first device number list */
	if((f_dir->dev_list = malloc(sizeof(dev_t))) == NULL)
		failed("allocate first device number list");
	f_dir->dev_list[0] = statbuf->st_dev;

	/* terminate linked list */
	f_dir->next = NULL;

	return flc;
}

/* next file in list */
char * fl_next(struct file_list_con *flc)
{
	size_t com_len;
	struct dirent *dir_e;

	if(flc->c_dir == NULL) return NULL;

	/* loop until a good path is found */
	while(true)
	{
		/* get the next file in the directory */
		if((dir_e = readdir(flc->dp)) != NULL)
		{
			/* ignore the current and parent directories */
			if((!strcmp(dir_e->d_name, ".")) || (!strcmp(dir_e->d_name, ".."))) continue;

			/* allocate space for the file path */
			com_len = flc->pre_len + strlen(dir_e->d_name) + 2;
			if(flc->space < com_len)
				if((flc->f_path = realloc(flc->f_path, flc->space = com_len)) == NULL)
					failed("allocate file path");

			/* put together path */
			if(flc->c_dir->path != NULL) sprintf(flc->f_path, "%s/%s", flc->c_dir->path, dir_e->d_name);
			else strcpy(flc->f_path, dir_e->d_name);

			if(get_stat(flc->follow_link, flc->f_path, flc->statbuf, flc->verbose)) continue;
			else break;
		}

		/* if the end of the directory has been reached */
		else
		{
			/* close current directory */
			closedir(flc->dp);
			flc->dp = NULL;

			/* open next directory */
			while(true)
			{
				/* if no more directories, return NULL */
				if((flc->c_dir = dr_next(flc->c_dir)) == NULL)
					return NULL;

				if((flc->dp = opendir(flc->c_dir->path)) == NULL)
				{if(flc->verbose) perror(flc->c_dir->path);}

				else
				{
					flc->pre_len = strlen(flc->c_dir->path);
					break;
				}
			}
		}
	}

	/* if directory, add to list of directories to process */
	if(S_ISDIR(flc->statbuf->st_mode)) dr_add(flc);

	return flc->f_path;
}

/* close file list */
void fl_close(struct file_list_con *flc)
{
	if(flc->dp != NULL) closedir(flc->dp);

	while(flc->c_dir != NULL) flc->c_dir = dr_next(flc->c_dir);

	free(flc->f_path);
	free(flc);
}

/* prepare hierarchy cache */
struct h_cache * hc_prep(bool follow_link, bool verbose)
{
	struct h_cache *hc;

	hc = malloc(sizeof(struct h_cache));

	hc->follow_link = follow_link;
	hc->verbose = verbose;

	hc->cache = NULL;
	hc->count = 0;

	return hc;
}

void hc_add(struct h_cache *hc, char *fn, struct stat *statbuf)
{
	DIR *dp;

	if((dp = opendir(fn)) == NULL)
	{perror(fn); return;}

	if((hc->cache = realloc(hc->cache, ++hc->count * sizeof(struct node_record))) == NULL)
		failed("allocate root node record");

	closedir(dp);
}

/* write file record */
void w_file_r(char *fn, struct stat *statbuf, struct opt_struct *opts)
{
	char *type;

	/* determine file type */
	if(opts->regular && S_ISREG(statbuf->st_mode)) type = "regular";
	else if(opts->directory && S_ISDIR(statbuf->st_mode)) type = "directory";
	else if(opts->chr_dev && S_ISCHR(statbuf->st_mode)) type = "character";
	else if(opts->blk_dev && S_ISBLK(statbuf->st_mode)) type = "block";
	else if(opts->symlink && S_ISLNK(statbuf->st_mode)) type = "symlink";
	else if(opts->fifo && S_ISFIFO(statbuf->st_mode)) type = "fifo";
	else return;

	/* write file type indicator */
	if(printf("file %s\n", type) < 0)
		failed("write file record header");

	/* write file path */
	if(printf("data %zu path\n%s\n", strlen(fn), fn) < 0) failed("write path field");

	/* write file size */
	if(opts->size)
		if(printf("size %ju\n", (uintmax_t)statbuf->st_size) < 0)
			failed("write size field");

	/* write modification time */
	if(opts->mtime)
		if(printf("mtime %ju %ld\n", (uintmax_t)statbuf->st_mtim.tv_sec, statbuf->st_mtim.tv_nsec) < 0)
			failed("write mtime field");

	/* end file record */
	if(putchar('\n') == EOF) failed("terminate file record");
}

/* process a directory */
void proc_dir(char *fn, struct stat *statbuf, struct opt_struct *opts)
{
	struct file_list_con *flc;

	if(fn != NULL) w_file_r(fn, statbuf, opts);

	if((flc = fl_prep(fn, statbuf, opts)) == NULL) return;

	while((fn = fl_next(flc)) != NULL) w_file_r(fn, statbuf, opts);

	fl_close(flc);
}

/* update an existing manifest file */
void update_manifest(char **fnames, struct opt_struct *opts)
{
	int i;
	char *fn;
	struct h_cache *hc;
	struct l_cache *lc;
	struct stat statbuf;
	struct man_rec mr;

	/* read header of input file */
	read_header();

	/* if adding records, prepare hierarchy cache */
	if(opts->add)
	{
		hc = hc_prep(opts->all_lnk, opts->verbose);

		for(i = 0; (fn = fnames[i]) != NULL; i++)
		{
			if(get_stat(opts->cmd_lnk, fn, &statbuf, true)) continue;
			hc_add(hc, fn, &statbuf);
		}
	}

	/* write header */
	if(puts("OUmanifest 1\n") == EOF) failed("write manifest header");

	/* process manifest records */
	while(mr_read(&mr))
	{
		/* if removing records, search through specified files */
		if(opts->remove)
		{
			for(i = 0; (fn = fnames[i]) != NULL; i++)
				if(check_path(fn, mr.path))
					break;

			if(fn == NULL) continue;
		}

		/* if adding records, remove path from cache */
		if(opts->add) hc_rem(mr.path);

		/* if updating modified records */
		if(opts->modified)
		{
			mr.mtime = statbuf.st_mtim;
			mr.atime = statbuf.st_atim;
		}

		/* write manifest record */
		mr_write(&mr);
	}

	/* if adding records */
	if(opts->add)
	{
		lc = lc_prep(hc);
		hc_close(hc);

		while((fn = lc_next(lc)) != NULL)
		{
			if(get_stat(opts->all_lnk, fn, &statbuf, opts->verbose)) continue;

			w_file_r(fn, &statbuf, opts);
		}

		lc_close(lc);
	}
}

/* create a new manifest */
void make_manifest(char **fnames, struct opt_struct *opts)
{
	int i;
	char *fn;
	struct stat statbuf;

	/* write header */
	if(puts("OUmanifest 1\n") == EOF) failed("write manifest header");

	/* process filenames on the command line */
	for(i = 0; (fn = fnames[i]) != NULL; i++)
	{
		/* get file status */
		if(get_stat(opts->cmd_lnk, fn, &statbuf, true)) continue;

		/* process a directory or record a file */
		if(S_ISDIR(statbuf.st_mode)) proc_dir(fn, &statbuf, opts);
		else w_file_r(fn, &statbuf, opts);
	}

	/* if no files are listed on the command line */
	if(i == 0)
	{
		/* get pwd metadata */
		if(stat(".", &statbuf) == -1)
		{perror(fn); exit(EXIT_FAILURE);}

		proc_dir(NULL, &statbuf, opts);
	}
}

/* parse file type options */
void file_type_opts(struct opt_struct *opts, char *arg)
{
	int i, c;

	for(i = 0; (c = arg[i]) != '\0'; i++)
		switch(c)
		{
			case 'r': opts->regular = true; break;
			case 'd': opts->directory = true; break;
			case 'c': opts->chr_dev = true; break;
			case 'b': opts->blk_dev = true; break;
			case 'l': opts->symlink = true; break;
			case 'f': opts->fifo = true; break;
			default: fprintf(stderr, "\"%c\" does not correspond to a file type\n", c); exit(EXIT_FAILURE);
		}
}

/* parse update options */
void update_opts(struct opt_struct *opts, char *arg)
{
	int i, c;

	for(i = 0; (c = arg[i]) != '\0'; i++)
		switch(c)
		{
			case 'a': opts->add = true; break;
			case 'r': opts->remove = true; break;
			case 'm': opts->modified = true; break;
			default: fprintf(stderr, "\"%c\" is not an update type\n", c); exit(EXIT_FAILURE);
		}
}

/* parse metadata options */
void metadata_opts(struct opt_struct *opts, char *arg)
{
	int i, c;

	for(i = 0; (c = arg[i]) != '\0'; i++)
		switch(c)
		{
			case 's': opts->size = true; break;
			case 'm': opts->mtime = true; break;
			default: fprintf(stderr, "\"%c\" does not correspond to a metadata type\n", c); exit(EXIT_FAILURE);
		}
}

int main(int argc, char **argv)
{
	int c;
	extern char *optarg;
	extern int opterr, optind, optopt;
	struct opt_struct opts = {false, false, false, false, false, false,
		false, false, false, false, false, false, false, false, false};

	/* the errno symbol is defined in errno.h */
	errno = 0;

	/* parse command line */
	while((c = getopt(argc, argv, "hvt:u:m:HL")) != -1)
		switch(c)
		{
			case 'h': help(); exit(EXIT_SUCCESS);
			case 'v': opts.verbose = true; break;
			case 't': file_type_opts(&opts, optarg); break;
			case 'u': opts.update = true; update_opts(&opts, optarg); break;
			case 'm': metadata_opts(&opts, optarg); break;
			case 'H': opts.cmd_lnk = true; opts.all_lnk = false; break;
			case 'L': opts.all_lnk = opts.cmd_lnk = true; break;
			case '?': exit(EXIT_FAILURE);
		}

	if(opts.update) update_manifest(argv + optind, &opts);
	else make_manifest(argv + optind, &opts);

	return EXIT_SUCCESS;
}
