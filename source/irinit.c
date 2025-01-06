/* $Id: init.c 58 2024-11-01 03:03:47Z nishi $ */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <blkid/blkid.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

void popdev_scan(void);

int main(int argc, char** argv){
	int fd;
	int i;
	pid_t pid;
	char* runlevel = NULL;
	char* root = NULL;
	char* rootfstype = NULL;
	char* fs[256];
	int fsincr = 0;
	FILE* fc;
	struct stat s;
	char* cmdline;
	char* oldroot;
	int cmdincr = 0;
	fs[0] = NULL;
	printf("\x1b[2J\x1b[1;1H");
	fflush(stdout);
	printf("Initramfs starting\n");
	mount("proc", "/proc", "proc", 0, "");
	fc = fopen("/proc/cmdline", "r");
	stat("/proc/cmdline", &s);
	cmdline = malloc(s.st_size + 1);
	cmdline[s.st_size] = 0;
	fread(cmdline, 1, s.st_size, fc);
	fclose(fc);
	if(cmdline[strlen(cmdline) - 1] == '\n') cmdline[strlen(cmdline) - 1] = 0;
	for(i = 0;; i++){
		if(cmdline[i] == ' ' || cmdline[i] == 0){
			char oldc = cmdline[i];
			char* cmd = cmdline + cmdincr;
			cmdline[i] = 0;
			cmdincr = i + 1;
			if(strcmp(cmd, "single") == 0){
				runlevel = "S";
			}else{
				char* equ = strdup(cmd);
				int j;
				for(j = 0; equ[j] != 0; j++){
					if(equ[j] == '='){
						equ[j] = 0;
						if(strcmp(equ, "root") == 0){
							if(root != NULL) free(root);
							root = strdup(equ + j + 1);
						}else if(strcmp(equ, "rootfstype") == 0){
							if(rootfstype != NULL) free(rootfstype);
							rootfstype = strdup(equ + j + 1);
						}
						break;
					}
				}
				free(equ);
			}
			if(oldc == 0) break;
		}
	}
	free(cmdline);
	if(rootfstype == NULL){
		char* line = NULL;
		size_t len = 0;
		int j;
		FILE* f = fopen("/proc/filesystems", "r");
		while(getline(&line, &len, f) != -1){
			int incr = 0;
			if(line[strlen(line) - 1] == '\n') line[strlen(line) - 1] = 0;
			for(j = 0;; j++){
				if(line[j] == ' ' || line[j] == '\t' || line[j] == 0){
					char oldc = line[j];
					line[j] = 0;
					if(strcmp(line + incr, "nodev") == 0) break;
					if(oldc == 0){
						fs[fsincr++] = strdup(line + incr);
						fs[fsincr] = NULL;
						break;
					}
					incr = j + 1;
				}
			}
		}
		free(line);
		fclose(f);
	}
	mount("sysfs", "/sys", "sysfs", 0, "");
	mount("tmpfs", "/tmp", "tmpfs", 0, "");
	if((pid = fork()) == 0){
		execl("/loadmod", "/loadmod", NULL);
		_exit(0);
	}
	waitpid(pid, 0, 0);
	sleep(5);
	if((pid = fork()) == 0){
		execl("/popdev", "/popdev", NULL);
		_exit(0);
	}
	waitpid(pid, 0, 0);
	if(root == NULL){
		fprintf(stderr, "root was not specified\n");
		return 1;
	}
	oldroot = root;
	root = blkid_evaluate_spec(root, NULL);
	free(oldroot);
	if(root == NULL){
		fprintf(stderr, "probe error\n");
		return 1;
	}
	if(rootfstype == NULL){
		for(i = 0; fs[i] != NULL; i++){
			if(mount(root, "/newroot", fs[i], MS_RDONLY, "") == 0) break;
		}
	}else{
		mount(root, "/newroot", rootfstype, MS_RDONLY, "");
	}
	mount("/dev", "/newroot/dev", "", MS_BIND, "");
	chdir("/newroot");
	umount("/proc");
	umount("/sys");
	umount("/tmp");
	mount(".", "/", NULL, MS_MOVE, NULL);
	chroot(".");
	chdir("/");
	mknod("/dev/initctl", S_IFIFO, 0);
	if(access("/dev/console", F_OK) != 0){
		printf("Creating /dev/console\n");
		if(mknod("/dev/console", S_IFCHR | S_IRUSR | S_IWUSR, makedev(5, 1)) != 0){
			printf("Error: %s\n", strerror(errno));
		}
	}
	if(access("/sbin/init", X_OK) != 0) return 0;
	fd = open("/dev/console", O_RDWR);
	if(fd < 0){
		fprintf(stderr, "Could not open console: %s\n", strerror(errno));
	}
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
	close(fd);
	execl("/sbin/init", "/sbin/init", runlevel, NULL);
	return 0;
}
