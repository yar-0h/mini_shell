#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include<sys/types.h>
#include<sys/wait.h>


// Maximum supported input size for this shell
#define FIELDBUFFERSIZE 80
#define COMMANDBUFFERSIZE 20
// Used to clear the shell using escape sequences
// https://stackoverflow.com/questions/55672661/what-this-character-sequence-033h-033j-does-in-c
#define clearTerminal() printf("\033[H\033[J")

// Signal Handler (used mostly for SIGINT)
void signalHandler(int signalID) {
    if (signalID == SIGINT) {
        printf("\nmini-shell terminated\n");
        exit(1);
    }
}

// Shell startup
void startShell()
{
    time_t currentTime;
    char hostname[1024];
    gethostname(hostname, sizeof(hostname));

    clearTerminal();
    printf("\n\n- - - - - - - - W E L C O M E  T O  M I N I _ S H E L L - - - - - - - -\n\n");
    printf("shell started: %s\n", ctime(&currentTime));
    printf("hosted on: %s", hostname);
    sleep(1);
    clearTerminal();
}

// Function to take input
char* queryInput() {
    char* rawCommand = malloc(FIELDBUFFERSIZE * sizeof(char));
    //char* rawCommand = NULL;
    printf("mini-shell>");
    fgets(rawCommand, FIELDBUFFERSIZE, stdin);
    return rawCommand;
}

int getCommandLength(char** parsedCommands) {
    int commandLength = 0;
    while (parsedCommands[commandLength] != NULL) {
        commandLength++;
    }
    return commandLength;
}


char** parseInput(char* rawCommand) {
    // allocating memory for the set of parsed commands to be passed

    char** parsedCommands = malloc(sizeof(char*) * COMMANDBUFFERSIZE);
    int i = 0;
    char delim[] = " \n";
    char* token = strtok(rawCommand, delim);

    // parsing raw command with spaces and filling string array
    while (token != NULL) {
        //parsedCommands = realloc(parsedCommands, sizeof(char *) * (COMMANDBUFFERSIZE));
        parsedCommands[i++] = token;
        token = strtok(NULL, delim);
    }
    parsedCommands[i] = NULL;
    return parsedCommands;
}

// returns 0 if there is no pipe among the parsed commands, returns the index of the pipe command if one exists.
int pipeCheck(char** parsedCommands) {
    int CommandLength = getCommandLength(parsedCommands);

    int pipeFlag = -1;
    char* pipe = "|";
    int i;
    for(i = 0; i < CommandLength; i++) {
        //printf("%s", parsedCommands[i]);
        if (!strcmp(parsedCommands[i], pipe)) {
            pipeFlag = i;
            break;
        }
    }
    return pipeFlag;
}

// Function where the system command is executed
void commandExecute(char** parsedCommands) {

    // Forking a child
    pid_t pid = fork();
    if (pid == 0) {
        execvp(parsedCommands[0], parsedCommands);
        printf("--command not found--\n");
        exit(0);
    }
    else {
        // Synchronization function
        wait(NULL);
    }
}


// Function where the piped system commands are executed
// Was incredibly stuck here but received amazing assistance from TA Yuehua, TA Magnus and the blog post at:
// http://www.rozmichelle.com/pipes-forks-dups/
void pipedCommandExecute(char** parsedCommands, int pipeIndex) {

    int commandLength = getCommandLength(parsedCommands);

    int i, j;
    int size1 = pipeIndex;
    int size2 = (commandLength - (pipeIndex + 1));
    char *firstCommandSet[size1+1];
    char *secondCommandSet[size2+1];

    // Building set of commands found prior to pipe
    for (i = 0; i < size1; i++) {
        firstCommandSet[i] = parsedCommands[i];
    }
    firstCommandSet[size1] = NULL;

    // Building set of commands found after pipe
    for (j = 0; j < size2; j++) {
        secondCommandSet[j] = parsedCommands[j + pipeIndex + 1];
    }
    secondCommandSet[size2] = NULL;

    //setting up pipe and file descriptors
    int fds[2]; // an array that will hold two file descriptors
    pipe(fds);  // populates fds with two file descriptors
    pid_t pid1 = fork(); // create first child process that is a clone of the parent

    if (pid1 < 0) {
        printf("\nError encountered with First Child Process\n");
        return;
    }
    if (pid1 == 0) {
        // copying output to fds[1] (the write end of pipe)
        dup2(fds[1], STDOUT_FILENO);
        // file descriptors no longer needed in child
        close(fds[1]);
        close(fds[0]);

        // run initial command (exit if anything goes wrong)
        if (execvp(firstCommandSet[0], firstCommandSet) < 0) {
            printf("\nError executing initial command\n");
            exit(0);
        }
    }
    else {
        pid_t pid2 = fork(); // create child process that is a clone of the parent
        if (pid2 < 0) {
            printf("\nError encountered with Second Child Process\n");
            return;
        }
        if (pid2 == 0) {
            // input from fds[0] (the read end of pipe)
            dup2(fds[0], STDIN_FILENO);
            // file descriptor no longer needed in child
            close(fds[1]);
            close(fds[0]);

            // run piped command (exit if anything goes wrong)
            if (execvp(secondCommandSet[0], secondCommandSet) < 0) {
                printf("\nError executing piped command\n");
                exit(0);
            }
        }
        else {
            close(fds[0]);
            close(fds[1]);
            wait(NULL);
            wait(NULL);
            return;
        }
    }
}

void miniShellHelp() {
    printf("\n- - M I N I _ S H E L L  H E L P - - \n"
           "_____________________________________\n\n");
    printf("-AVAILABLE BUILT IN FUNCTIONS-\n"
           "_____________________________________\n");
    printf("EXIT\n"
           "____\n"
           "-call this to terminate the shell\n"
           "-it SHOULD only terminate the current outermost shell\n\n");
    printf("CD\n"
           "__\n"
           "-works similarly to the regular 'cd' we know and love\n"
           "-use this command with a directory address to make that the current working directory\n"
           "-you can also use '..' to move up a directory\n\n");
    printf("HELP\n"
           "____\n"
           "-call this to print out a list of MINI_SHELL's builtin functions\n"
           "-you used it to get here in the first place!\n"
           "-calling it a second time will break the shell (ONLY JOKING)\n\n");
    printf("JANKENPO\n"
           "______\n"
           "-call this to play MINI_SHELL's Rock-Paper-Scissors game\n"
           "-the game responds to 'rock', 'paper', 'scissors' and 'quit'\n"
           "-use this game to waste your time in an unsatisfying way!\n\n");
    printf("TIMER\n"
           "_____\n"
           "-call this to know the time you've spent in MINI_SHELL\n"
           "-use this to measure the time you've wasted in an unsatisfying way!\n\n");
    printf("------------------------------------------\n"
           "|  this mini-shell written by y.simmons  |\n"
           "------------------------------------------\n\n");
}

// This function is used to generate a random number between 0 - 2 for the purpose of the rockpaperscissors game.
// I don't think it is super random, but I believe it is unpredictable enough for this purpose.
int cpuJanKenGen() {
    int randInt = (rand() % (3));
    return randInt;
}

void miniShellJanken() {
    srand(time(0));
    char *jankenMove = malloc(FIELDBUFFERSIZE * sizeof(char));
    char *quitCall = "q\n";
    int cpuMove;
    int score = 0;
    int cpuScore = 0;

    printf("\n----get ready for ROCK-PAPER-SCISSORS----\n"
           "_________________________________________\n\n");
    while (1) {
        printf("choose to play 'ROCK', 'PAPER', or 'SCISSORS' (or 'Q' to quit)\n");
        fgets(jankenMove, FIELDBUFFERSIZE, stdin);
        cpuMove = cpuJanKenGen();
        if (!strcasecmp(jankenMove, quitCall)) {
            return;
        }
        else {
            printf("you:%s"
                   "cpu:", jankenMove);
            switch (cpuMove) {
                case 0:
                    //CPU chooses ROCK
                    printf("ROCK.JPG\n");
                    if (!strcasecmp(jankenMove, "rock\n")) {
                        printf("TIE GAME\n");
                        break;
                    } else if (!strcasecmp(jankenMove, "paper\n")) {
                        printf("YOU WIN\n");
                        score++;
                        break;
                    } else if (!strcasecmp(jankenMove, "scissors\n")) {
                        printf("YOU LOSE\n");
                        cpuScore++;
                        break;
                    } else {
                        printf("I'm sorry, that move didn't make any sense to me\n");
                        break;
                    }
                    break;
                case 1:
                    //CPU chooses PAPER
                    printf("PAPER.TXT\n");
                    if (!strcasecmp(jankenMove, "paper\n")) {
                        printf("TIE GAME\n");
                        break;
                    } else if (!strcasecmp(jankenMove, "scissors\n")) {
                        printf("YOU WIN\n");
                        score++;
                        break;
                    } else if (!strcasecmp(jankenMove, "rock\n")) {
                        printf("YOU LOSE\n");
                        cpuScore++;
                        break;
                    } else {
                        printf("I'm sorry, that move didn't make any sense to me\n");
                        break;
                    }
                    break;
                case 2:
                    //CPU chooses SCISSORS
                    printf("SCISSORS.EXE\n");
                    if (!strcasecmp(jankenMove, "scissors\n")) {
                        printf("TIE GAME\n");
                        break;
                    } else if (!strcasecmp(jankenMove, "rock\n")) {
                        printf("YOU WIN\n");
                        score++;
                        break;
                    } else if (!strcasecmp(jankenMove, "paper\n")) {
                        printf("YOU LOSE\n");
                        cpuScore++;
                        break;
                    } else {
                        printf("I'm sorry, that move didn't make any sense to me\n");
                        break;
                    }
                    break;
                default:
                    break;
            }
            printf("\nyour score:%d                cpu_score:%d\n", score, cpuScore);
            printf("play again? ('yes' to play again, 'no' to quit)\n");
            while (1) {
                fgets(jankenMove, FIELDBUFFERSIZE, stdin);
                if (!strcasecmp(jankenMove, "yes\n")) {
                    break;
                } else if (!strcasecmp(jankenMove, "no\n")) {
                    return;
                } else (printf("I'm sorry, I didn't understand that command\n"));
            }
        }
    }
    free(jankenMove);
}

void miniShellTimer(time_t shellStartTime) {
    time_t currentTime;
    double timeSpent;

    time(&currentTime);
    timeSpent = difftime(currentTime, shellStartTime);
    printf("you've been using this shell for about %f seconds\ngood job\n\n", timeSpent);

}

// checks to see whether the command is a built in one, and returns a corresponding integer
int builtInCommandCheck(char** parsedCommands) {
    int cmdFlag = 0;
    char* cmdExit = "exit";
    char* cmdCD = "cd";
    char* cmdHelp = "help";
    char* cmdJanken = "jankenpo";
    char* cmdTimer = "timer";

    if (!strcmp(parsedCommands[0], cmdExit)) {
        cmdFlag = 1;
    }
    else if (!strcmp(parsedCommands[0], cmdCD)) {
        cmdFlag = 2;
    }
    else if (!strcmp(parsedCommands[0], cmdHelp)) {
        cmdFlag = 3;
    }
    else if (!strcmp(parsedCommands[0], cmdJanken)) {
        cmdFlag = 4;
    }
    else if (!strcmp(parsedCommands[0], cmdTimer)) {
        cmdFlag = 5;
    }
    return cmdFlag;
}

// Function for executing builtin commands
int builtInCommand(char** parsedCommands, int builtInSwitch)
{
    char s[100];
    switch (builtInSwitch) {
        case 1:
            exit(0);
            break;
        case 2:
            chdir(parsedCommands[1]);
            // printing new working directory
            printf("now working in: %s\n", getcwd(s, 100));
            break;
        case 3:
            miniShellHelp();
            break;
        case 4:
            miniShellJanken();
            break;
        default:
            break;
    }
    return 0;
}


int main() {
    signal(SIGINT, signalHandler);
    char *rawCommands;
    char **parsedCommands;

    int pipeFlag;
    int builtInFlag;

    time_t start_t;

    setbuf(stdout, 0);
    time(&start_t);
    startShell();

    while (1) {
        // receiving input
        rawCommands = queryInput();

        // processing input
        // the parseInput function, using the raw input will return an array of strings,
        // parsed using the raw input's spaces
        parsedCommands = parseInput(rawCommands);

        if (*parsedCommands != NULL) {
            // the pipeCheck function, using the parsed input will return:
            // -1 if it is a simple command with no pipe
            // the index of the pipe if it is a command including a pipe
            pipeFlag = pipeCheck(parsedCommands);
            builtInFlag = builtInCommandCheck(parsedCommands);

            // execute
            if (pipeFlag == -1) {
                if (builtInFlag == 5) {
                    miniShellTimer(start_t);
                } else if (builtInFlag != 0) {
                    builtInCommand(parsedCommands, builtInFlag);
                } else { commandExecute(parsedCommands); }
            }
            if (pipeFlag > -1)
                pipedCommandExecute(parsedCommands, pipeFlag);
        }
        free(rawCommands);
        free(parsedCommands);
    }
    return 0;
}

