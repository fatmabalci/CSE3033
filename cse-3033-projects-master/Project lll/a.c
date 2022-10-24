/*
    Authors:
        - Fatma BALCI - 150119744
        - Alper DOKAY - 150119746
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>

// This Default Size represents the initial size of dynamically allocated array
#define DEFAULT_SIZE 8

// numberOfWords defined to keep the latest index for allocated array
int numberOfWords = 0;
// currentSize is used to expand the array by multiplying it by 2 each time it exceeds
int currentSize = 8;
// this mutex is generally defined to be used to lock the process in threads
pthread_mutex_t lock;

// This struct represents a word parsed and have a linked list to keep the file names that the word in
struct Word {
    char* wordName;
    struct FileNode* fileNode;
};

// This is the dynamically allocated array
struct Word *words[DEFAULT_SIZE];

// This is created to be used in main to iterate over each file read in the directory
struct FileNode {
    char* fileName;
    struct FileNode *next;
};

// This is the struct to be given to thread function for each thread
struct ProcessFileItem{
    char* dirName;  // this keeps the directory name given in the beginning
    char* fileName;  // this is the fileName for the specified thread to be executed
};

// This is the method inserts a new fileNode to fileNode linked list
void insertFileNode(struct FileNode** head_node, char* fileName){
    
    // allocate new node's memory
    struct FileNode* newNode = (struct FileNode*) malloc(sizeof(struct FileNode));

    // take the last node as head first to be iterated later
    struct FileNode *lastNode = *head_node;

    // assign values to newly created node
    newNode->fileName = fileName;
    // set the next item as NULL since the node will be added to the end
    newNode->next = NULL;

    // check if the head node is null
    if (*head_node == NULL) {
        // If the head node is empty, then make the new node head node
        *head_node = newNode;
        return;
    }

    // if the head is not null, iterate over the list until the last item
    while (lastNode->next != NULL) {
        char* currentFileName = lastNode->fileName;
        // if the fileName is already added, then return null to cancel this insertion
        if (strcmp(currentFileName, fileName) == 0){
            // This file was already added to given word
            return;
        }
        // go to the next item
        lastNode = lastNode->next;
    }

    // This file was already added to given word (assumption: there is only head node)
    if (strcmp(lastNode->fileName, fileName) == 0) {
        return;
    }

    // put newly created node to the end
    lastNode->next = newNode;

    return;
}

// This is the method to print the file list, given the head node
void printFileList(struct FileNode *node){
    // iterate through each item in the linked list
    while (node != NULL) {
        // print the fileName
        printf("\t-%s \n", node->fileName);
        // go to the next node
        node = node->next;
    }
}

// This is the method to print the word list in the dynamically allocated array 
void printWordList(){
    // print the header for the list
    printf("\t Word List: \n");
    int i;
    // iterate through each item in the allocated array
    for (i = 0; i < numberOfWords; i++){
        // print the word
        printf("\t-%s \n", words[i]->wordName);
    }
}

// This is the method to add a word to dynamically allocated array
void insertWord(char* wordName, char* fileName){
    
    // allocate memory in the array
    words[numberOfWords] = (struct Word*) malloc(sizeof(struct Word));
    // allocate memory for the word
    words[numberOfWords]->wordName = malloc(sizeof(char *));
    // set the word
    strcpy(words[numberOfWords]->wordName, wordName);
    // set the word's fileNode as empty initially
    words[numberOfWords]->fileNode = NULL;
    // call insertion method for the fileName to be inserted
    insertFileNode(&words[numberOfWords]->fileNode, fileName);
    
    // increase the number of words
    numberOfWords++;
    return;
}

// This is the method for inserting a fileName to word's file linked list
void insertFileToWord(int index, char* fileName) {
    // re-order the parameters and call the insertion function
    insertFileNode(&words[index]->fileNode, fileName);
}

// This is the method to search for a word in the dynamically allocated array 
int searchWord(char* givenWordName){
    // set loc as -1 in the beginning
    int loc = -1;

    int i;

    // go through each word
    for (i = 0; i < numberOfWords; i++) {
        char *wordName = words[i]->wordName;
        // check if the word exists
        if (strcmp(wordName, givenWordName) == 0){
            // if so, set the loc to the index and break
            loc = i;
            break;
        }
    }

    // return loc, if it is -1, it means the word does not exists
    return loc;
}

// This is the method for threads
void* processFile(void *arg) {

    // take the model created for this thread
    struct ProcessFileItem *item = arg;

    // print the assignment
    printf("MAIN THREAD: Assigned \"%s\" to worker thread %ld. \n", item->fileName, pthread_self());

    // allocate memory for the filePath
    char* filePath = malloc(sizeof(char*));
    // form the filePath to be opened below in read mode
    strcat(filePath, item->dirName);
    strcat(filePath, "/");
    strcat(filePath, item->fileName);

    // open the file
    FILE *fp = fopen(filePath, "r");
    // set the buffer as 512 to read it in a line
    char buffer[512];

    // get the values line by line
    while(fgets(buffer, 512, fp) != NULL){
        // split the line by whitespaces
        char *token = strtok(buffer, " ");
        
        // go through each word
        while (token != NULL) {
            // check if the word exists
            int index = searchWord(token);
            if (index != -1) {
                // print the index if it does not exists
                printf("THREAD %ld: The word \"%s\" has already located at index %d.\n", pthread_self(), token, index);
                // call the fileName insertion method for this word
                insertFileToWord(index, item->fileName);
            }
            else {
                // lock the thread until it adds the new value
                pthread_mutex_lock(&lock);
                // check if the numberOfWords exceeds the current size
                if (numberOfWords >= currentSize) {
                    // multiply the size by 2
                    currentSize = currentSize * 2;
                    // do reallocation for the dynamically allocated array
                    *words = (struct Word*) realloc(*words, currentSize * sizeof(struct Word));
                    // print the allocation
                    printf("THREAD %ld: Re-allocated array of %d pointers. \n", pthread_self(), currentSize);
                }
                // insert the word
                insertWord(token, item->fileName);
                // unlock the thread since it took its slot and added the word there
                pthread_mutex_unlock(&lock);
                // print the insertion with index
                printf("THREAD %ld: Added \"%s\" at index %d.\n", pthread_self(), token, (numberOfWords-1));
            }
            
            // get the next item in the split
            token = strtok(NULL, " ");
        }
    }

    // close the file
    fclose(fp);

    // exit from the thread by returning NULL
    return NULL;
}

// This is the main method to run whole application
int main(int argc, char *argv[]){

    int i;
    // this is the variable to keep the number of threads given
    int numberOfThreads;
    // this is the variable to keep the directory name
    char* dirName;
    // keep isDirSuccess & isNumberSuccess to check given arguments
    int isDirSuccess = 0;
    int isNumberSuccess = 0;
    // keep available threads to be updated later
    int availableThreads = 0;
    // this is something we have seen in the lab
    void *status;
    // keep this to have a counter for the number of files
    int fileCounter = 0;

    // keep fileList as linked list
    struct FileNode* fileListNode = NULL;

    // go through each item in the arguments and get the values
    for (i = 0; i < argc; i++) {
        // check if the tag is "-d"
        if (strcmp(argv[i], "-d") == 0){
            // then assign the next item which is directory name to directory value
            dirName = argv[i + 1];
            i++;
            // assign the dir success as 1 meaning the validation is OK
            isDirSuccess = 1;
        }
        // check if the tag is "-n"
        if (strcmp(argv[i], "-n") == 0){
            // then assign the next item which is the number of threads
            // convert it to int from string
            numberOfThreads = atoi(argv[i + 1]);
            i++;
            // assign the number success as 1 meaning the validation is OK
            isNumberSuccess = 1;
        }
    }


    // check if expected values are given to the program
    if (!isDirSuccess || !isNumberSuccess) {
        // print errors and exit!
        printf("ERROR: Invalid arguments \n");
        printf("USAGE: ./a.out -d <directoryName> -n >#ofThreads>\n");
        return 0;
    }

    // set available threads to be used in the loop
    availableThreads = numberOfThreads;

    // create the thread array to be implemented in the loop later 
    pthread_t tid[numberOfThreads];

    // Read file names
    DIR *d;
    struct dirent *dir;

    // get the directory
    d = opendir(dirName);

    // get into the files if d is not NULL
    if(d) {
        // read the files in the directory till every item is read
        while ((dir = readdir(d)) != NULL) {
            // this was occuring in my machine to have . and .. by default
            // to prevent having such fileNames, I put these conditions
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
                continue;
            }
            // allocate memory for dir name
            char *dirItemName = malloc(sizeof(char*));
            // set the dir name
            strcpy(dirItemName, dir->d_name);
            // insert the fileNode to the fileList
            insertFileNode(&fileListNode, dirItemName);
            // increment the counter
            fileCounter++;
        }
        // close the dir
        closedir(d);
    }
    
    // keep the error as 0 in the beginning to check later in the thread creation
    int error = 0;
    // have the phthread index as 0 by default to be used in thread array
    int pthread_index = 0;

    // try to initiate the mutex, end the program if it does not
    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\nMutex creation failed!");
        return 1;
    }

    // check if there exists a thread available
    while (availableThreads > 0) {
        // check if the file node is not null
        if (fileListNode != NULL) {
            // allocate memory for a temporary fileName to be copied into the ProcessFileItem struct
            char *value = malloc(sizeof(char*));
            // set the value
            strcpy(value, fileListNode->fileName);
            // go the next element
            fileListNode = fileListNode->next;

            // create the struct object for the thread function, allocate memory
            struct ProcessFileItem *item = (struct ProcessFileItem*) malloc(sizeof(struct ProcessFileItem));
            // allocate memory for dir name
            item->dirName = malloc(sizeof(char*));
            // allocate memory for file name
            item->fileName = malloc(sizeof(char*));
            // set dir name
            strcpy(item->dirName, dirName);
            // set file name
            strcpy(item->fileName, value);

            // try to create the thread with the created obj
            error = pthread_create(&(tid[pthread_index]), NULL, &processFile, (void *)item);

            // if an error occurs, print it and continue
            if (error != 0) {
                printf("\nNew thread creation was not successful!, Error: %s", strerror(error));
            }

            // increment the thread index
            pthread_index++;
            // decrement the available threads
            availableThreads--;
        }
        
    }
    
    int j;
    // go through each thread and join one by one
    for (j = 0; j < numberOfThreads; j++){
        pthread_join(tid[j], &status);
    }

    // destroy the mutex initiated once the processes are ended
    pthread_mutex_destroy(&lock);

    // print the latest report
    printf("MAIN THREAD: All done (successfully read %d words with %d threads from %d files).\n", numberOfWords, numberOfThreads, fileCounter);

    // exit
    return 0;
}