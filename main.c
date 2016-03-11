#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_WDIR_LENGTH 257
#define MAX_PARAMS 15
#define MAX_BG_JOBS 5

typedef int bool;
#define true 1
#define false 0

int jobs_running = 0;

struct job
{
	char job_name[MAX_WDIR_LENGTH];
	int local_pid;
	int external_pid;
	char status;
	bool in_use;
};

struct job jobs[MAX_BG_JOBS];

void get_cwd(char * cwd){
	getcwd(cwd, MAX_WDIR_LENGTH);
}

void change_cwd(char * location){
	char cwd[MAX_WDIR_LENGTH];
	getcwd(cwd, MAX_WDIR_LENGTH);
	if(chdir(location) < 0)
	{
		printf("Error: %s: Directory not found\n", location );
	}
}

//Returns true if expected number of arguments equals given
bool num_args_check(int expected, int given){
	int i;
	if(expected==given) return true;
	if(given > expected) i = 0;
	if(given < expected) i = -1;
	printf("Error: Too %s arguments given.\n", (i<0) ? "few" : "many");
	return false;
}

void print_current_jobs(){
	int i = 0;
	int num_running = 0;
	for (i = 0; i < MAX_BG_JOBS; i++){
		if(jobs[i].in_use){
			num_running++;
			printf("%d [%c]: %s\n",i,jobs[i].status, jobs[i].job_name);
		}
	}
	printf("Total Background Jobs: %d\n", num_running );
}

void execute_cmd(char * p_arg_array[MAX_PARAMS]){
	pid_t child_pid = fork();
		if (child_pid == 0)
		{
			execvp(p_arg_array[0], p_arg_array);
			perror("execvp");   /* execve() only returns on error */
	    }
		else
		{
			int	status = 0;
			waitpid (child_pid, &status,0);
		}
}

int available_pid(){
	int k;
	for (k = 0; k < MAX_BG_JOBS; k++){
		if(jobs[k].in_use == false){
			return k;
		}
	}
	return -1;
}

void execute_bg_cmd(char * bg_arg_array[MAX_PARAMS]){
	pid_t child_pid = fork();
		if (child_pid == 0)
		{
			execvp(bg_arg_array[0], bg_arg_array);
			perror("execvp");
	    }
		else
		{
			int x = available_pid();
			strcpy(jobs[x].job_name, bg_arg_array[0]);
			jobs[x].external_pid = child_pid;
			jobs[x].status = 'R';
			jobs[x].in_use = true;
			jobs_running++;
		}
}

void remove_job(int job){
	jobs[job].in_use = false;
	jobs_running--;
}

void kill_process( char * process ){
	//printf("Killing process: %s\n", process );
	int num = (int)process[0] - 48;
	if (num < 0 || num >= MAX_BG_JOBS)
	{
		printf("Error: Not a valid process number.\n");
		return;
	}
	if(jobs[num].in_use == false){
		printf("Error: Process %d not running.\n", num);
		return;
	}
	kill(jobs[num].external_pid, SIGKILL);
	remove_job(num);
	printf("Killed process: %d\n", num);
}

void resume_job(char * process){
	int num = (int)process[0] - 48;
	if (num < 0 || num > 4)
	{
		printf("Error: Can only specify jobs from 0 to 4.\n");
		return;
	}
	if(jobs[num].status == false)
	{
		printf("Error: Process %d not running\n", num);
		return;
	}
	kill(jobs[num].external_pid, SIGCONT);
	jobs[num].status = 'R';
}

void pause_job(char * process){
	int num = (int)process[0] - 48;
	if (num < 0 || num > 4)
	{
		printf("Error: Can only specify jobs from 0 to 4.\n");
		return;
	}
	if(jobs[num].status == false)
	{
		printf("Error: Process %d not running\n", num);
		return;
	}
	kill(jobs[num].external_pid, SIGSTOP);
	jobs[num].status = 'S';
	printf("Killed process: %d\n", num );
}

void init_jobs(){
	int i = 0;
	for (i = 0; i < MAX_BG_JOBS; i++){
		jobs[i].in_use = false;
		jobs[i].local_pid = i;
	}
}


int main ( void )
{
	printf("Type 'exit' to quit shell.\n");
	init_jobs();
	for (;;)
	{
		//Format shell prompt
		char cwd[MAX_WDIR_LENGTH+1] = "";
		get_cwd(cwd);
		strcat(cwd,">");
		char * cmd = readline (cwd);
		
		//Check for ended background jobs
		pid_t returned_child;
		int status = 0;
		int lp;
		for (lp = 0; lp < MAX_BG_JOBS; lp++){
			if(jobs[lp].in_use){
				if( (int)(returned_child = waitpid(jobs[lp].external_pid, &status, WNOHANG)) > 0 ){
					printf("Process %d has finished: %s\n",lp, jobs[lp].job_name);
					remove_job(lp);
				}
			}
		}
		
		//Reset argument arrays
		char *p_arg_array[MAX_PARAMS] = {};
		char *bg_arg_array[MAX_PARAMS] = {};
		char arg_array[MAX_PARAMS][MAX_WDIR_LENGTH] = {};

		//Parse input
		char * token = strtok(cmd, " ");
		int num_args = 0;
		int k = 0;
		while(token != NULL && num_args < 15)
		{
			strcpy(arg_array[num_args], token);
			//printf("%s\n", token);
			token = strtok(NULL, " ");
			num_args++;
		}

		//Too many arguments given
		if(token != NULL){
			printf("Error: Too many arguments, 15 argument max.\n");
			continue;
		}

		// If empty input restart
		if (num_args == 0){ 
			//printf("\n");
			continue;
		}

		free (cmd);

		//Create pointer array for easy passing
		for(k = 0; k < num_args; k++)
		{
			p_arg_array[k] = arg_array[k];
		}

		if (strcmp(p_arg_array[0], "pwd") == 0)
		{
			//printf("%s\n", "caught pwd");
			if(num_args_check(1,num_args)){
				char path[MAX_WDIR_LENGTH]; 
				get_cwd(path);
				printf("%s\n", path );
			}
			continue;
		}

		if (strcmp(p_arg_array[0], "cd") == 0)
		{
			//printf("%s\n", "caught cd");
			if(num_args_check(2,num_args)){
				change_cwd(p_arg_array[1]);
			}
			continue;
		}

		if (strcmp(p_arg_array[0], "bg") == 0)
		{
			//printf("%s\n", "caught bg");
			if(jobs_running >= 5)
			{ 
				printf("Error: limit of background jobs reached, wait for one to finish or kill a process.\n");
				continue;
			}

			if (num_args > 1)
			{
				int z;
				for(z = 0; z < num_args-1; z++){
					bg_arg_array[z] = arg_array[z+1];
				}
				execute_bg_cmd(bg_arg_array);
			}
			else
			{
				printf("Error: No arguments for 'bg' recieved.\n");
			}
			continue;
		}

		if (strcmp(p_arg_array[0], "bglist") == 0)
		{
			//printf("%s\n", "caught bglist");
			if(num_args_check(1, num_args)) print_current_jobs();
			continue;
		}

		if (strcmp(p_arg_array[0], "bgkill") == 0)
		{
			//printf("%s\n", "caught bgkill");
			if(num_args_check(2, num_args)) kill_process(p_arg_array[1]);
			continue;
		}

		if (strcmp(p_arg_array[0], "exit") == 0)
		{
			//printf("%s\n", "caught exit");
			if(num_args_check(1, num_args)) exit(0);
			continue;
		}

		if (strcmp(p_arg_array[0], "start") == 0)
		{
			//printf("%s\n", "caught exit");
			if(num_args_check(2, num_args)) resume_job(p_arg_array[1]);
			continue;
		}

		if (strcmp(p_arg_array[0], "stop") == 0)
		{
			//printf("%s\n", "caught exit");
			if(num_args_check(2, num_args)) pause_job(p_arg_array[1]);
			continue;
		}
		
		execute_cmd(p_arg_array);
	}	
}