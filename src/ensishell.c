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
#include <fcntl.h>
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
#include <stdio.h>



 

typedef struct _cell{
    int pid;
    char* command;
    struct _cell* next;
}*list_jobs;



void add_job(list_jobs* l,int pid,char* command){
    list_jobs p = calloc(1,sizeof(p));
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
void remove_job( list_jobs*l , int pid){
	if(*l!=NULL){
		if ((*l)->pid == pid){
			list_jobs p = *l;
			*l=(*l)->next;
			free(p);
			p=NULL;
			return;
		}
		list_jobs courant = (*l)->next;
		list_jobs precedent = *l;
		while (courant != NULL){
			if (courant->pid == pid){
				list_jobs p = courant;
				precedent->next = courant->next;
				free(p);
				p=NULL;
				return;
			}
			precedent = courant;
			courant = courant->next;
		}
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
				
				int pid = waitpid(-1,NULL,WNOHANG);
				remove_job(&l_jobs,pid);
				print_jobs(l_jobs);
				continue;
			}
		}
		int p[2];
		if (pipe(p) < 0){
			perror("pipe:");
			exit(1);
		}
		
		
		int g ;

		switch(child_pid=fork()){
			case -1:
				perror("fork:");
				break;
			case 0:
				if (l->seq[1] != NULL){
					close(p[0]);
					dup2(p[1],1);
				}
				
				execvp(*l->seq[0],&(*l->seq[0]));
			default:
				if (l->seq[0]!= NULL){
					if(l->seq[1]){
						if ((g = fork()) == 0){
							close(p[1]);
							dup2(p[0],0);
							execvp(*l->seq[1],&(*l->seq[1]));
							perror("Echec deuxieme commande");
							exit(EXIT_FAILURE);
							
						}

					}

				}
				close(p[0]);
				close(p[1]);
				if(!l->bg){
					if (l->seq[1]){
						waitpid(g,NULL,0);
					}
					waitpid(child_pid,NULL,0);
				}else{
					waitpid(child_pid, NULL, WNOHANG);
					add_job(&l_jobs,child_pid,*l->seq[0]);
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
		// for (int i=0; l->seq[i]!=0; i++) {
		// 	char **cmd = l->seq[i];
		// 	printf("seq[%d]: ", i);
        //                 for (int j=0; cmd[j]!=0; j++) {
        //                         printf("'%s' ", cmd[j]);
        //                 }
		// 	printf("\n");
		// } 
	}

}
