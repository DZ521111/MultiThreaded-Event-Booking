#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_FILENAME_LEN 256
#define MAX_FILES 100
#define MAX_CONTENT_LEN 100

int main() {
    int child1, child2;
    int pipefd1[2], pipefd2[2];

    // create two pipes
    if (pipe(pipefd1) == -1 || pipe(pipefd2) == -1) {
        perror("Error creating pipes");
        exit(EXIT_FAILURE);
    }

    // First child process
    child1 = fork();
    if (child1 == -1) {
        perror("Error forking child 1");
        exit(EXIT_FAILURE);
    } else if (child1 == 0) {
        // Sending files from first child to second child.
        DIR* dir;
        struct dirent* dir_entry;
        dir = opendir("d1");
        if (dir == NULL) {
            perror("Error opening directory d1");
            exit(EXIT_FAILURE);
        }

        // Reading content of files from directory d1.
        char filenames1[MAX_FILES][MAX_FILENAME_LEN];
        char contents1[MAX_FILES][MAX_CONTENT_LEN];
        int num_files = 0;
	//here readdir reads all the directory in the dir directory
        while ((dir_entry = readdir(dir)) != NULL && num_files < MAX_FILES) {
            char path[MAX_FILENAME_LEN+5]; // just to ensure no overflow +5 is done otherwise +3 would also work.
            sprintf(path, "d1/%s", dir_entry->d_name);
            FILE* file = fopen(path, "r");
            if (file != NULL) {
                strcpy(filenames1[num_files], dir_entry->d_name);
		//byteRead contains the number of bytes read by fread fxn.
                size_t bytesRead = fread(contents1[num_files], sizeof(char), MAX_CONTENT_LEN, file);
                contents1[num_files][bytesRead] = '\0';
                fclose(file);
                num_files++;
            }
        }
        closedir(dir);

        // writing filenames and contents to pipe
        // close reading end of pipe 1 while writing to pipe 1
        close(pipefd1[0]); 
        write(pipefd1[1], &num_files, sizeof(num_files));
        for (int i = 0; i < num_files; i++) {
            write(pipefd1[1], filenames1[i], sizeof(filenames1[i]));
            write(pipefd1[1], contents1[i], sizeof(contents1[i]));
        }
        // close writing end of pipe 1
        close(pipefd1[1]);

        // Put first child process on sleep
        sleep(3);

        // Receiving files from directory d2 to directory d1.

        // Read filenames and contents from pipe
        char filenames2[MAX_FILES][MAX_FILENAME_LEN];
        char contents2[MAX_FILES][MAX_CONTENT_LEN];
        // close writing end of pipe 2
        close(pipefd2[1]); 
        read(pipefd2[0], &num_files, sizeof(num_files));
        for (int i = 0; i < num_files; i++) {
            read(pipefd2[0], filenames2[i], sizeof(filenames2[i]));
            read(pipefd2[0], contents2[i], sizeof(contents2[i]));
        }
        // close reading end of pipe 2
        close(pipefd2[0]); 

        // create files in directory d1 and write contents we received from second child
        for (int i = 0; i < num_files; i++) {
            char path[MAX_FILENAME_LEN+5];
            sprintf(path, "d1/%s", filenames2[i]);
            FILE* file = fopen(path, "w");
            if (file != NULL) {
                fputs(contents2[i], file);
                fclose(file);
            }
        }
        exit(EXIT_SUCCESS);
    }

    // Second child process
    child2 = fork();
    if (child2 == -1) {
        perror("Error forking child 2");
        exit(EXIT_FAILURE);
    } else if (child2 == 0) {
        // Receiving files from directory d1 to directory d2.

        // Read filenames and contents from pipe
        int num_files = 0;
        char filenames1[MAX_FILES][MAX_FILENAME_LEN];
        char contents1[MAX_FILES][MAX_CONTENT_LEN];

        // read filenames and contents from pipe
        // close writing end of pipe 1
        close(pipefd1[1]); 
        read(pipefd1[0], &num_files, sizeof(num_files));
        for (int i = 0; i < num_files; i++) {
            read(pipefd1[0], filenames1[i], sizeof(filenames1[i]));
            read(pipefd1[0], contents1[i], sizeof(contents1[i]));
        }
        // close reading end of pipe 1
        close(pipefd1[0]); 

        // create files in directory d2 and write contents we received from first child
        for (int i = 0; i < num_files; i++) {
            char path[MAX_FILENAME_LEN+5];
            sprintf(path, "d2/%s", filenames1[i]);
            FILE* file = fopen(path, "w");
            if (file != NULL) {
                fputs(contents1[i], file);
                fclose(file);
            }
        }


        // Sending files from second child to first child.

        DIR* dir;
        struct dirent* dir_entry;
        dir = opendir("d2");
        if (dir == NULL) {
            perror("Error opening directory d2");
            exit(EXIT_FAILURE);
        }

        // Reading content of files from directory d2.
        num_files = 0;
        char filenames2[MAX_FILES][MAX_FILENAME_LEN];
        char contents2[MAX_FILES][MAX_CONTENT_LEN];
        while ((dir_entry = readdir(dir)) != NULL && num_files < MAX_FILES) {
            char path[MAX_FILENAME_LEN+5];
            sprintf(path, "d2/%s", dir_entry->d_name);
            FILE* file = fopen(path, "r");
            if (file != NULL) {
                strcpy(filenames2[num_files], dir_entry->d_name);
                size_t bytesRead = fread(contents2[num_files], sizeof(char), MAX_CONTENT_LEN, file);
                contents2[num_files][bytesRead] = '\0';
                fclose(file);
                num_files++;
            }
        }

        closedir(dir);

        // write filenames and contents to pipe
        // close reading end of pipe 2
        close(pipefd2[0]); 
        write(pipefd2[1], &num_files, sizeof(num_files));
        for (int i = 0; i < num_files; i++) {
            write(pipefd2[1], filenames2[i], sizeof(filenames2[i]));
            write(pipefd2[1], contents2[i], sizeof(contents2[i]));
        }
        // close writing end of pipe 2
        close(pipefd2[1]); 

        exit(EXIT_SUCCESS);
    }
    
    // Parent Process

    // wait for child processes to finish
    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);

    return 0;
}
