/*****************************************************
 * Copyright Grégory Mounié 2008-2015                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "variante.h"
#include "readcmd.h"



#include <sys/types.h>
#include <sys/wait.h>

#ifndef VARIANTE
#error "Variante non défini !!"
#endif

/* Guile (1.8 and 2.0) is auto-detected by cmake */
/* To disable Scheme interpreter (Guile support), comment the
 * following lines.  You may also have to comment related pkg-config
 * lines in CMakeLists.txt.
 */

#if USE_GUILE == 1
#include <libguile.h>



void handler(int signum) 
{ 
   waitpid(WAIT_ANY, NULL, WNOHANG);
  
} 
 

typedef struct _cell{
    int status;
    int pid;
    char* command;
    struct _cell* next;
}*list_jobs;



void add_first(list_jobs* l,int status,int pid,char* command){
    list_jobs p = calloc(1,sizeof(p));
    p->status = status;
    p->pid = pid;
    p->command = calloc(strlen(command),sizeof(char));
	p->next = NULL;
    strcpy(p->command,command);
    if(*l==NULL) *l=p;
    else{
        p->next = *l;
        *l=p;
    }
}
void print_jobs(list_jobs l){
	list_jobs p = l;
	while (p!=NULL){
		printf("pid=%d ; commande = %s\n",p->pid,p->command);
		p=p->next;
	}
}




int question6_executer(char *line)
{
	/* Question 6: Insert your code to execute the command line
	 * identically to the standard execution scheme:
	 * parsecmd, then fork+execvp, for a single command.
	 * pipe and i/o redirection are not required.
	 */
	printf("Not implemented yet: can not execute %s\n", line);

	/* Remove this line when using parsecmd as it will free it */
	free(line);
	
	return 0;
}

SCM executer_wrapper(SCM x)
{
        return scm_from_int(question6_executer(scm_to_locale_stringn(x, 0)));
}
#endif


void terminate(char *line) {
#if USE_GNU_READLINE == 1
	/* rl_clear_history() does not exist yet in centOS 6 */
	clear_history();
#endif
	if (line)
	  free(line);
	printf("exit\n");
	exit(0);
}


int main() {
        printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

#if USE_GUILE == 1
        scm_init_guile();
        /* register "executer" function in scheme */
        scm_c_define_gsubr("executer", 1, 0, 0, executer_wrapper);
#endif
	list_jobs l_jobs = 0;
	while (1) {
		struct cmdline *l;
		char *line=0;
		// int i, j;
		char *prompt = "ensishell>";
		

		/* Readline use some internal memory structure that
		   can not be cleaned at the end of the program. Thus
		   one memory leak per command seems unavoidable yet */
		line = readline(prompt);
		if (line == 0 || ! strncmp(line,"exit", 4)) {
			terminate(line);
		}

#if USE_GNU_READLINE == 1
		add_history(line);
#endif


#if USE_GUILE == 1
		/* The line is a scheme command */
		if (line[0] == '(') {
			char catchligne[strlen(line) + 256];
			sprintf(catchligne, "(catch #t (lambda () %s) (lambda (key . parameters) (display \"mauvaise expression/bug en scheme\n\")))", line);
			scm_eval_string(scm_from_locale_string(catchligne));
			free(line);
                        continue;
                }
#endif

		/* parsecmd free line and set it up to 0 */
		l = parsecmd( & line);
		pid_t child_pid;
		
		if(l && (*l->seq)){
			if(!strcmp(*l->seq[0],"jobs")){
				print_jobs(l_jobs);
			}
		}

		switch(child_pid=fork()){
			case -1:
				perror("fork:");
				break;
			case 0:
				
				execvp(*l->seq[0],*l->seq);
				
				break;
			default:

				if(!l->bg){
    				waitpid(child_pid,NULL,0);
				}else{
					signal(SIGCHLD,handler);
					add_first(&l_jobs,0,child_pid,*l->seq[0]);
				}
				
  			
		}

		/* If input stream closed, normal termination */
		if (!l) {
		  
			terminate(0);
		}
		
		//print_jobs(l_jobs);
		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}

		if (l->in) printf("in: %s\n", l->in);
		if (l->out) printf("out: %s\n", l->out);
		if (l->bg) printf("background (&)\n");

		/* Display each command of the pipe */
		/* for (i=0; l->seq[i]!=0; i++) {
			char **cmd = l->seq[i];
			printf("seq[%d]: ", i);
                        for (j=0; cmd[j]!=0; j++) {
                                printf("'%s' ", cmd[j]);
                        }
			printf("\n");
		} */
	}

}
