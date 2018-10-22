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
 * The main shell function
 */
main() {
    char **args;
    int saved_in = dup(0);
    int saved_out = dup(1);
// Loop forever
    while(1) {
        int status;
        int pid =  waitpid(-1, &status, WNOHANG);
        if(pid>0){
            printf("PID %d has finished\n", pid);
        }else{
            char* buff[100];
            getcwd(buff, 100);
            // Print out the prompt and get the input
            printf("%s$ ", buff);
            dup2(saved_in, 0);	
	    args = getaline();
            
	    // No input, continue
            if(args[0] == NULL)
                continue;

            // Check for internal shell commands, such as exit
            if(internal_command(args))
                continue;

            findCharacters(&args, saved_in, saved_out, 0);
	}
    }
}	

void findCharacters(char*** args, int saved_in, int saved_out, int cancelCommand){
	int index=0;
	while((*args)[index] != NULL){
		if((*args)[index][0] == ';'){
			free((*args)[index]);
			(*args)[index] = NULL;
			dup2(saved_out, 1);
			if(!cancelCommand){
				int output;
   				int input;
    				char *output_filename;
    				char *input_filename;
				input = redirect_input(*args, &input_filename);
				output = redirect_output(*args, &output_filename);
				do_command(*args, 1, input, input_filename, output, output_filename);
			}
			*args = &(*args)[index+1];
			findCharacters(args, saved_in, saved_out, 0);
			return;
		}else if((*args)[index][0] == '|' && (*args)[index+1][0] != '|'){
			free((*args)[index]);		
			(*args)[index] = NULL;
			if(!cancelCommand){
				int output;
   				int input;
    				char *output_filename;
    				char *input_filename;
				input = redirect_input(*args, &input_filename);
				output = redirect_output(*args, &output_filename);
				
				int fd[2];
				pipe(fd);
				dup2(fd[1], 1);
				close(fd[1]);
				
				do_command(*args, 1, input, input_filename, output, output_filename);
				*args = &(*args)[index+1];
				dup2(fd[0], 0);
				close(fd[0]);
			}
			findCharacters(args, saved_in, saved_out, 0);
			return;
		}else if((*args)[index][0] == '&' && (*args)[index+1] != NULL && (*args)[index+1][0] == '&'){
			free((*args)[index]);		
			(*args)[index] = NULL;
			free((*args)[index+1]);		
			(*args)[index+1] = NULL;
			dup2(saved_out, 1);
			int status;
			if(!cancelCommand){
				int output;
   				int input;
    				char *output_filename;
    				char *input_filename;
				input = redirect_input(*args, &input_filename);
				output = redirect_output(*args, &output_filename);
				status = do_command(*args, 1, input, input_filename, output, output_filename);
			}
			int cancelNextCommand = 0;
			if(status!=0){
				cancelNextCommand = 1;
			}
			*args = &(*args)[index+2];
			findCharacters(args, saved_in, saved_out, cancelNextCommand);		
			return;
		}else if((*args)[index][0] == '|' && (*args)[index+1][0] == '|'){
			free((*args)[index]);		
			(*args)[index] = NULL;
			free((*args)[index+1]);		
			(*args)[index+1] = NULL;
			dup2(saved_out, 1);
			int status;
			if(!cancelCommand){
				int output;
   				int input;
    				char *output_filename;
    				char *input_filename;
				input = redirect_input(*args, &input_filename);
				output = redirect_output(*args, &output_filename);
				status = do_command(*args, 1, input, input_filename, output, output_filename);
			}
			int cancelNextCommand = 0;
			if(status==0){
				cancelNextCommand = 1;
			}
			*args = &(*args)[index+2];
			findCharacters(args, saved_in, saved_out, cancelNextCommand);		
			return;
		}
	index++;
	}
	dup2(saved_out, 1);
	if(!cancelCommand){
		int output;
 		int input;
    		char *output_filename;
    		char *input_filename;
		input = redirect_input(*args, &input_filename);
		output = redirect_output(*args, &output_filename);
		int block = ampersand(*args);
		do_command(*args, block, input, input_filename, output, output_filename);
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
		
	// Execute the command
        result = execvp(args[0], args);
        exit(-1);
    }

  // Wait for the child process to complete, if necessary
    if(block) {
      //  printf("Waiting for child, pid = %d\n", child_id);
        result = waitpid(child_id, &status, 0);
	return status;
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

