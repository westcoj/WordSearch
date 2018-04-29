//CODY WEST
//CIS 452 Programming Assignment 2

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <sys/wait.h>
#include <signal.h>

using namespace std;

//VALUES FOR PIPES
#define READ 0
#define WRITE 1

//MAX FILE NAME LENGTH AND NUMBER OF FILES
#define MAXNAME 1024
#define MAXFILES 10

//METHOD FOR TAKING DOWN ALL CHILDREN
void sigChild(int);

/*************************************************************************
* Searches a given file for a given word and records the number
* of times that word is found. Used by children of parent.
*
* @param searchWord The word to look for
* @param fileName The file to search
* @return Number of times a word is found
************************************************************************/
int numWords(string searchWord, string fileName) {
    ifstream fileIn;
    int result = 0;
    string currWord;


    fileIn.open(fileName.c_str());

    //Alerts parent of bad file name
    if (!fileIn) {
        perror("File Read Error");
        return -1;
    }

    while (fileIn >> currWord) {
        if (searchWord == currWord)
            result++;
    }

    fileIn.close();

    //cout << "I am searching for: |" << searchWord << "| in: |" << fileName << "| got: |" << result << "|\n";

    return result;
}

/*************************************************************************
* In use when parent informs children to stop
************************************************************************/
void sigChild(int x){
    exit(1);
};

/*************************************************************************
 * Method run by children to continuously search a file for given words.
 * Ends when parent is given a non alphabetical entry.
 *
 * @param pitc[] Parent to child pipe
 * @param ctp[] Child to parent pipe
 * @return Number of times a word is found
************************************************************************/
void childWorker(int ptc[], int ctp[]) {

    close(ptc[WRITE]);
    close(ctp[READ]);


    //cout << "A worker has spawned!\n" ;
    char childWord[MAXNAME];

    //GET FILE NAME TO BE USED
    read(ptc[READ], (void *) childWord, (size_t) sizeof(childWord));
    string badFileString(childWord);
    //string fileString = badFileString.substr(0, badFileString.length() - 1);
    //cout << "Bad: " << badFileString << "\n";
    //cout << "Fixed: " << fileString << "\n";

    //SEND ACCEPTANCE
    int fileGotNum = 1;
    write(ctp[WRITE], &fileGotNum, (size_t) sizeof(int));

    //LOOP OF SEARCHING FOR WORDS
    while (1) {
        read(ptc[READ], (void *) childWord, (size_t) sizeof(childWord));
        string childString(childWord);
        //REMOVE INCORRECT WORDING (EXTRA CHARACTER CAUSES READ ERROR)
        string realCString = childString.substr(0, childString.length() - 1);

        //int num1 = 5;
        //fgets (test,MAXNAME,stdin);
        int num1 = numWords(realCString, badFileString);
        //printf("Child 1 here: ");
        //cout << childString << " " << num1 << "\n";
        //write (STDOUT_FILENO, (const void *) test, (size_t) strlen(test)+1);
        write(ctp[WRITE], &num1, (size_t) sizeof(int));
    }

}

/*************************************************************************
 * Sets up all children neccesary up to 10 files, and provides parent
 * loop for searching files for a specific word.
************************************************************************/
int main(int argc, char *argv[]) {
    int fileNumbers = argc - 1;
    if (fileNumbers == 0) {
        cout << "No files given to program\n";
        return 0;
    }

    if (fileNumbers >= MAXFILES) {
        cout << "Too many files provided\n";
        return 0;
    }

    cout << "Files Given: " << fileNumbers << "\n";

    //PIDS for all children, parent, and specific exit pid;
    pid_t pids[fileNumbers];
    pid_t pidP = getpid();
    pid_t pidE;

    //Array of pipes to be handed out to children
    int pipesPTC[fileNumbers][2];
    int pipesCTP[fileNumbers][2];

    //Set up children with fork() calls
    for (int k = 0; k < fileNumbers; k++) {
        if (pipe(pipesCTP[k])) {
            perror("Pipe Issue");
            exit(1);
        }

        if (pipe(pipesPTC[k])) {
            perror("Pipe Issue");
            exit(1);
        }

        if ((pids[k] = fork()) < 0) {
            perror("Forking Issues");
            exit(0);
        } else if (pids[k] == 0) {
            signal(SIGUSR1,sigChild);
            childWorker(pipesPTC[k], pipesCTP[k]);
            return 0;
        }

    }

    //PRINT THE PIDS
    cout << "Parent PID: " << pidP << "\n";
    for (int k = 0; k < fileNumbers; k++) {
        cout << "Child " << k << " PID: " << pids[k] << " Parent PID: " << pidP << "\n";
    }

    //HANDLE THE PARENTS PIPES

    for (int k = 0; k<fileNumbers; k++) {
        close(pipesCTP[k][WRITE]);
        close(pipesPTC[k][READ]);
    }

    //WRITE TO ALL THE CHILDREN (FILE NAMES)
    for (int k = 0; k<fileNumbers; k++) {
        char fileName[MAXNAME];
        strcpy(fileName,argv[k+1]);
        cout << "Next FileName: ";
        puts(argv[k+1]);
        cout << "\n";
        write(pipesPTC[k][WRITE], (const void *) fileName, (size_t) strlen(fileName) + 1);
    }

    int fileNameGot = 0;

    //READ FROM ALL THE CHILDREN (ACCEPTANCE SIGNAL)
    for (int k = 0; k < fileNumbers; k++) {
        read(pipesCTP[k][READ], &fileNameGot, (size_t) sizeof(int));
        if (fileNameGot != 1) {
            perror("File Name Piping Issues");
            exit(1);
        }

    }

    //NOW GET THE USER'S INPUT (THE LOOP)
    char sWord[MAXNAME];
    locale loc;
    while (1) {
        cout <<"Enter the word to search files for: \n";
        fgets(sWord, MAXNAME, stdin);
        string unRealPWord(sWord);
        string sWordString = unRealPWord.substr(0, unRealPWord.length() - 1);

        //EXIT HANDLER VIA SIGNAL
        for (std::string::iterator it = sWordString.begin(); it != sWordString.end(); ++it) {
            if (!std::isalpha(*it, loc)) {

                int status;

                for (int k = 0; k < fileNumbers; k++) {
                    kill(pids[k],SIGUSR1);
                    pidE = wait(&status);
                    cout << "Child has exited with ID: " << pidE << "\n";
                }

                cout << "Children have died, let's keep em that way \n";
                return 0;
            }

        }

        //WRITE WORD TO ALL PIPES
        for (int k = 0; k < fileNumbers; k++) {
            write(pipesPTC[k][WRITE], (const void *) sWord, (size_t) strlen(sWord) + 1);
        }

        //READ RESULTS FROM ALL CHILDREN
        int results;
        for (int k = 0; k<fileNumbers; k++) {
            read(pipesCTP[k][READ], &results, (size_t) sizeof(int));
            
            //IF CHILD CAN'T READ A FILE (SHOULD BE MOVED UP)
            if(results<0){
                perror("Error With File Names, Verify Entry");
                int status;

                for (int k = 0; k < fileNumbers; k++) {
                    kill(pids[k],SIGUSR1);
                    pidE = wait(&status);
                    cout << "Child has exited with ID: " << pidE << "\n";
                }

                cout << "Children have died, let's keep em that way \n";
                return 0;
            }
            cout << "Child |" << k << "| found |" << results << "| of word |" << sWordString << "| in file |"
                 << argv[k + 1] << "| \n";
        }
        
    }
}








