/*
 * This code implements a simple shell program
 * It supports the internal shell command "exit", 
 * backgrounding processes with "&", input redirection
 * with "<" and output redirection with ">".
 * However, this is not complete.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
extern char **getaline();


/*
 * Handle exit signals from child processes
 */
void sig_handler(int signal) {
//  int status;
//  int result = wait(&status);

  printf("Wait returned %d\n", signal);
}



/*
 * The main shell function
 */ 
main() {
  int i;
  char **args; 
  int result;
  int block;
  int output;
  int input;
  int pipe;
  char *output_filename;
  char *input_filename;
  char *pipe_command;

  // Set up the signal handler
//  sigset(SIGCHLD,  sig_handler);
 
// Loop forever
  while(1) {
	int status;   
int pid =  waitpid(-1, &status, WNOHANG);
char* buff[100];
    getcwd(buff, 100);

if(pid>0){
	printf("PID %d has finished\n", pid);
//	printf("%s$ ", buff);	
}else{
    char* buff[100];
    getcwd(buff, 100);
    // Print out the prompt and get the input
    printf("%s$ ", buff);
    args = getaline();

    // No input, continue
    if(args[0] == NULL)
      continue;

    // Check for internal shell commands, such as exit
    if(internal_command(args))
      continue;

    // Check for an ampersand
    block = ampersand(args);

    // Check for redirected input
    input = redirect_input(args, &input_filename);

    switch(input) {
    case -1:
      printf("Syntax error!\n");
      continue;
      break;
    case 0:
      break;
    case 1:
      printf("Redirecting input from: %s\n", input_filename);
      break;
    }

    // Check for redirected output
    output = redirect_output(args, &output_filename);

    switch(output) {
    case -1:
      printf("Syntax error!\n");
      continue;
      break;
    case 0:
      break;
    case 1:
      printf("Redirecting output to: %s\n", output_filename);
      break;
    case 2:
      printf("Redirecting >> output to: %s\n", output_filename);
      break;
    }

    //Check for pipe
//    pipe = redirect_pipe(args, &pipe_command);

    // Do the command
    do_command(args, block, 
	       input, input_filename, 
	       output, output_filename);
  }
}
}

/*
 * Check for ampersand as the last argument
 */
int ampersand(char **args) {
  int i;

  for(i = 1; args[i] != NULL; i++) ;
  if(args[i-1][0] == '&') {
    free(args[i-1]);
    args[i-1] = NULL;
    return 0;
  }
  
  return 1;
}

/* 
 * Check for internal commands
 * Returns true if there is more to do, false otherwise 
 */
int internal_command(char **args) {
  if(strcmp(args[0], "exit") == 0) {
    exit(0);
  }else if(strcmp(args[0], "cd") == 0){
    chdir(args[1]);
  }

  return 0;
}

/* 
 * Do the command
 */
int do_command(char **args, int block,
	       int input, char *input_filename,
	       int output, char *output_filename) {
  
  int result;
  pid_t child_id;
  int status;

//sigset(SIGTTOU, sig_no);



//struct sigaction new_action, old_action;

//new_action.sa_handler = sig_no;
//new_action.sa_flags = 0;
//int act = sigaction(SIGTTIN, &new_action, NULL);
//if(!act){
//	printf("createdAction\n");
//}else{
//printf("failedAction\n");
//}
//sigset(SIGCHLD, sig_handler);
//sigset(SIGTTOU, SIG_IGN);
//sigset(SIGTTIN, SIG_IGN);
//sigset(SIGCHLD, SIG_IGN);
 

  // Fork the child process
  child_id = fork();

  // Check for errors in fork()
  switch(child_id) {
  case EAGAIN:
    perror("Error EAGAIN: ");
    return;
  case ENOMEM:
    perror("Error ENOMEM: ");
    return;
  }
 if(child_id == 0) {
if(!block)
 setpgid(child_id, 0);
	 // Set up redirection in the child process
    if(input)
      freopen(input_filename, "r", stdin);

    if(output==1)
      freopen(output_filename, "w+", stdout);
    if(output==2)
      freopen(output_filename, "a", stdout);

// set back to default for potential children	
//      signal (SIGINT, SIG_DFL);
//      signal (SIGQUIT, SIG_DFL);
//      signal (SIGTSTP, SIG_DFL);
//      signal (SIGTTIN, SIG_DFL);
//      signal (SIGTTOU, SIG_DFL);
//      signal (SIGCHLD, SIG_DFL);


//	printf("%s\n", args[0]);
//	printf("%s\n", args[1]);

//    if(pipe){
//      printf("Args 0 = %s\n", args[0]);
//      printf("Args 2 = %s\n", args[2]);
//      result = execvp(args[0], args);
//    }else{	
      // Execute the command
      
      result = execvp(args[0], args);
//    }
   exit(-1);
  }

  // Wait for the child process to complete, if necessary
  if(block) {
    printf("Waiting for child, pid = %d\n", child_id);
    result = waitpid(child_id, &status, 0);
   }else{		
       printf("%d started in background\n", child_id);
   }
}

/*
 * Check for input redirection
 */
int redirect_input(char **args, char **input_filename) {
  int i;
  int j;

  for(i = 0; args[i] != NULL; i++) {

    // Look for the <
    if(args[i][0] == '<') {
      free(args[i]);

      // Read the filename
      if(args[i+1] != NULL) {
	*input_filename = args[i+1];
      } else {
	return -1;
      }

      // Adjust the rest of the arguments in the array
      for(j = i; args[j-1] != NULL; j++) {
	args[j] = args[j+2];
      }

      return 1;
    }
  }

  return 0;
}

/*
 * Check for output redirection
 */
int redirect_output(char **args, char **output_filename) {
  int i;
  int j;

  for(i = 0; args[i] != NULL; i++) {
	
    // Look for the >
    if(args[i][0] == '>') {
     	//Look for the second > 
     if(args[i+1][0] == '>'){
	free(args[i]);
	free(args[i+1]);
	 
	// Get the filename 
      	if(args[i+2] != NULL) {
		*output_filename = args[i+2];
      	} else {
		return -1;
      	}

      	// Adjust the rest of the arguments in the array
      	for(j = i; args[j-1] != NULL; j++) {
		args[j] = args[j+3];
      	}

	return 2;
      }else{
      //Only one > was found 
	free(args[i]);

      // Get the filename 
      if(args[i+1] != NULL) {
	*output_filename = args[i+1];
      } else {
	return -1;
      }

      // Adjust the rest of the arguments in the array
      for(j = i; args[j-1] != NULL; j++) {
	args[j] = args[j+2];
      }

      return 1;
	}
   }
  }

  return 0;
}

/*
 * Check for pipe redirection
 */
//int redirect_pipe(char **args, char **pipe_command) {
//  int i;
//  int j;
//
//  for(i = 0; args[i] != NULL; i++) {
//
//    // Look for the |
//    if(args[i][0] == '|') {
//      free(args[i]);
//
//      // Read the command
//      if(args[i+1] != NULL) {
//	*pipe_command = args[i+1];
//      } else {
//	return -1;
//      }
//
//      // Adjust the rest of the arguments in the array
//      for(j = i; args[j-1] != NULL; j++) {
//	args[j] = args[j+2];
//      }
//
//      return 1;
//    }
//  }
//
//  return 0;
//}


