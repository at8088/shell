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
#include <wordexp.h>


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



/*********** Liste des processus en fond de tâche ***************/ 

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

int is_in_bg_jobs(list_jobs l_jobs,int pid){
	
	list_jobs p = l_jobs;
	while (p!=NULL)
	{
		if (p->pid == pid)
		{
			return 1;
		}
		p=p->next;
		
	}
	return 0;
	
	}
	


/***********************************************************************/





/******************************************************************** */


int question6_executer(char *line)
{
	struct cmdline * l;
	l = parsecmd(&line);
	switch (fork())
	{
	case -1:
		perror("fork");
		break;
	case 0:
		execvp(*l->seq[0],&(*l->seq[0]));
	
	default:
		wait(0);
		break;
	}
	

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


/*Liste des processus lancés en tache de fond */
list_jobs l_jobs = 0;

void traitant(int sig){

	int pid = waitpid(-1,0,WNOHANG);
	if(pid > 0 && is_in_bg_jobs(l_jobs,pid)){
		remove_job(&l_jobs,pid);
		printf("\nprocess [%d] is done.\n",pid);
	}

}



int main() {
        printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

#if USE_GUILE == 1
        scm_init_guile();
        /* register "executer" function in scheme */
        scm_c_define_gsubr("executer", 1, 0, 0, executer_wrapper);
#endif
	// list_jobs l_jobs = 0;
	while (1) {
		struct cmdline *l;
		char *line=0;
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
		// wordexp_t options ;
		// if (l->seq)
		// {
		// 	if (l->seq[0][1])
		// 	{
		// 		wordexp(l->seq[0][1],&options,0);
		// 	}
			
		// }
		
		pid_t child_pid;

		if(l && (*l->seq)){
			if(!strcmp(*l->seq[0],"jobs")){
				waitpid(-1,NULL,WNOHANG);
				// remove_job(&l_jobs,pid);
				print_jobs(l_jobs);
				continue;
			}
		}
		int p[2];
		if (pipe(p) < 0){
			perror("pipe:");
			exit(1);
		}

		int sortie_pipe ;
		int out_fd = 0;
		int in_fd  = 0;
		switch(child_pid=fork()){
			case -1:
				perror("fork:");
				break;
			case 0:
				if (l->seq[1] != NULL){
					close(p[0]);
					dup2(p[1],1);
				}
				if (l->out){
					out_fd = open(l->out,O_CREAT|O_WRONLY,S_IRUSR|S_IWUSR); 
					dup2(out_fd,STDOUT_FILENO);
					close(out_fd);
				}
				if (l->in){
					in_fd=open(l->in,O_RDONLY);
					dup2(in_fd,STDIN_FILENO);
					close(in_fd);
				}
				// char ** op = &(*l->seq[0]);
				// if(options.we_wordv){
				// 	op = options.we_wordv;
				// }
				
				execvp(*l->seq[0],&(*l->seq[0]));
			default:
				if (l->seq[0]!= NULL){
					if(l->seq[1]){
						if ((sortie_pipe = fork()) == 0){
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
				// if (l->seq[0][1])
				// {
				// 	wordfree(&options);
					
				// }
				if(!l->bg){
					if (l->seq[1]){
						waitpid(sortie_pipe,NULL,0);
					}
					waitpid(child_pid,NULL,0);
					signal(SIGCHLD,traitant);
				}else{
					waitpid(child_pid, NULL, WNOHANG);
					add_job(&l_jobs,child_pid,*l->seq[0]);
				}
		}
		

		/* If input stream closed, normal termination */
		if (!l) {
			terminate(0);
		}
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
        //                         printf("l->seq[%d][%d] '%s' ",i,j, cmd[j]);
        //                 }
		// 	printf("\n");
		// } 
	}

}
