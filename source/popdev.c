#define _XOPEN_SOURCE 500

#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <dirent.h>
#include <sys/sysmacros.h>

extern char** environ;

char* popdev_concat(const char* str1, const char* str2){
	char* str = malloc(strlen(str1) + strlen(str2) + 1);
	strcpy(str, str1);
	strcpy(str + strlen(str1), str2);
	str[strlen(str1) + strlen(str2)] = 0;
	return str;
}

char* popdev_concat3(const char* str1, const char* str2, const char* str3){
	char* tmp = popdev_concat(str1, str2);
	char* str = popdev_concat(tmp, str3);
	free(tmp);
	return str;
}

int main(int argc, char** argv){
	int i;
	DIR* dir = opendir("/sys/class");
	if(dir != NULL){
		char* cleanone[1];
		cleanone[0] = NULL;
		environ = cleanone;
		struct dirent* d;
		while((d = readdir(dir)) != NULL){
			if(strcmp(d->d_name, "..") == 0 || strcmp(d->d_name, ".") == 0) continue;
			char* p = popdev_concat("/sys/class/", d->d_name);
			DIR* subdir = opendir(p);
			if(subdir != NULL){
				struct dirent* subd;
				while((subd = readdir(subdir)) != NULL){
					if(strcmp(subd->d_name, "..") == 0 || strcmp(subd->d_name, ".") == 0) continue;
					char* path = popdev_concat3(p, "/", subd->d_name);
					char* ueventp = popdev_concat(path, "/uevent");
					FILE* f = fopen(ueventp, "r");
					if(f != NULL){
						char* linebuf = malloc(1);
						linebuf[0] = 0;
						char* buf = malloc(2);
						buf[1] = 0;
						while(1){
							fread(buf, 1, 1, f);
							if(feof(f) || buf[0] == '\n'){
								if(strlen(linebuf) > 0){
									int j;
									for(j = 0; linebuf[j] != 0; j++){
										if(linebuf[j] == '='){
											linebuf[j] = 0;
											break;
										}
									}
									setenv(linebuf, linebuf + j + 1, 1);
								}
								free(linebuf);
								linebuf = malloc(1);
								linebuf[0] = 0;
								if(feof(f)) break;
							}else{
								char* tmp = linebuf;
								linebuf = popdev_concat(tmp, buf);
								free(tmp);
							}
						}
						if(getenv("DEVNAME") != NULL){
							int i;
							char* devname = strdup(getenv("DEVNAME"));
							int first_index = 0;
							chdir("/dev");
							for(i = 0; devname[i] != 0; i++){
								if(devname[i] == '/'){
									devname[i] = 0;
									mkdir(devname + first_index, 0755);
									chdir(devname + first_index);
									first_index = i + 1;
								}
							}
							free(devname);
							char* devpath = popdev_concat("/dev/", getenv("DEVNAME"));
							mode_t mode = 0660;
							if(getenv("DEVMODE") != NULL){
								char* end;
								char* p = getenv("DEVMODE");
								mode = strtol(p, &end, 8);
							}
							if(strcmp(d->d_name, "block") == 0){
								mode |= S_IFBLK;
							}else{
								mode |= S_IFCHR;
							}
							char* maj = getenv("MAJOR");
							char* min = getenv("MINOR");
							mknod(devpath, mode, makedev(atoi(maj ? maj : "1"), atoi(min ? min : "1")));
							chmod(devpath, mode);
							free(devpath);
						}
						free(linebuf);
						free(buf);
						fclose(f);
					}
					free(ueventp);
					free(path);
				}
				closedir(subdir);
			}
			free(p);
		}
		closedir(dir);
	}else{
		fprintf(stderr, "popdev could not open /sys/class\n");
	}
}
