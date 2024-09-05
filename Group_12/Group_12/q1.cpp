#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_FILENAME_LEN 256
#define MAX_FILES 100
#define MAX_CONTENT_LEN 100

void readDirectoryContents(const char *dirPath, char filenames[MAX_FILES][MAX_FILENAME_LEN], char contents[MAX_FILES][MAX_CONTENT_LEN], int *num_files) {
    DIR* dir;
    struct dirent* dir_entry;
    dir = opendir(dirPath);
    if (dir == NULL) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }
    *num_files = 0;
    while ((dir_entry = readdir(dir)) != NULL && *num_files < MAX_FILES) {
        if (dir_entry->d_name[0] == '.') continue; // Skip hidden files and directories
        char path[MAX_FILENAME_LEN+5];
        sprintf(path, "%s/%s", dirPath, dir_entry->d_name);
        FILE* file = fopen(path, "r");
        if (file != NULL) {
            strcpy(filenames[*num_files], dir_entry->d_name);
            size_t bytesRead = fread(contents[*num_files], sizeof(char), MAX_CONTENT_LEN, file);
            contents[*num_files][bytesRead] = '\0';
            fclose(file);
            (*num_files)++;
        }
    }
    closedir(dir);
}

void writeFiles(const char *dirPath, char filenames[MAX_FILES][MAX_FILENAME_LEN], char contents[MAX_FILES][MAX_CONTENT_LEN], int num_files) {
    for (int i = 0; i < num_files; i++) {
        char path[MAX_FILENAME_LEN+5];
        sprintf(path, "%s/%s", dirPath, filenames[i]);
        FILE* file = fopen(path, "w");
        if (file != NULL) {
            fputs(contents[i], file);
            fclose(file);
        }
    }
}

void writeToPipe(int pipefd[2], char filenames[MAX_FILES][MAX_FILENAME_LEN], char contents[MAX_FILES][MAX_CONTENT_LEN], int num_files) {
    close(pipefd[0]); // Close reading end
    write(pipefd[1], &num_files, sizeof(num_files));
    for (int i = 0; i < num_files; i++) {
        write(pipefd[1], filenames[i], sizeof(filenames[i]));
        write(pipefd[1], contents[i], sizeof(contents[i]));
    }
    close(pipefd[1]); // Close writing end
}

void readFromPipe(int pipefd[2], char filenames[MAX_FILES][MAX_FILENAME_LEN], char contents[MAX_FILES][MAX_CONTENT_LEN], int *num_files) {
    close(pipefd[1]); // Close writing end
    read(pipefd[0], num_files, sizeof(*num_files));
    for (int i = 0; i < *num_files; i++) {
        read(pipefd[0], filenames[i], sizeof(filenames[i]));
        read(pipefd[0], contents[i], sizeof(contents[i]));
    }
    close(pipefd[0]); // Close reading end
}

int main() {
    int child1, child2;
    int pipefd1[2], pipefd2[2];

    if (pipe(pipefd1) == -1 || pipe(pipefd2) == -1) {
        perror("Error creating pipes");
        exit(EXIT_FAILURE);
    }

    child1 = fork();
    if (child1 == -1) {
        perror("Error forking child 1");
        exit(EXIT_FAILURE);
    } else if (child1 == 0) {
        char filenames1[MAX_FILES][MAX_FILENAME_LEN];
        char contents1[MAX_FILES][MAX_CONTENT_LEN];
        int num_files;
        
        // Read from d1 and send to pipe
        readDirectoryContents("d1", filenames1, contents1, &num_files);
        writeToPipe(pipefd1, filenames1, contents1, num_files);
        
        sleep(3); // Simulate work
        
        // Receive from pipe and write to d1
        readFromPipe(pipefd2, filenames1, contents1, &num_files);
        writeFiles("d1", filenames1, contents1, num_files);
        
        exit(EXIT_SUCCESS);
    }

    child2 = fork();
    if (child2 == -1) {
        perror("Error forking child 2");
        exit(EXIT_FAILURE);
    } else if (child2 == 0) {
        char filenames2[MAX_FILES][MAX_FILENAME_LEN];
        char contents2[MAX_FILES][MAX_CONTENT_LEN];
        int num_files;
        
        // Receive from pipe and write to d2
        readFromPipe(pipefd1, filenames2, contents2, &num_files);
        writeFiles("d2", filenames2, contents2, num_files);
        
        // Read from d2 and send to pipe
        readDirectoryContents("d2", filenames2, contents2, &num_files);
        writeToPipe(pipefd2, filenames2, contents2, num_files);
        
        exit(EXIT_SUCCESS);
    }
    
    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);

    return 0;
}

