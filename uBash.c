#include "uBash.h"

// inizializza il parametro da mandare a execv e lancia execv;
void execute(char *path, char *command, char *options)
{
	char *comando[] = {command, NULL}; // metto dentro "comando[]" il nome del comando da eseguire
	for (int i = 1; comando[i - 1] != NULL; i++)
	{
		comando[i] = strtok_r(options, " \n", &options); // aggiungo a "comando[]" le opzioni del comando da eseguire
	}
	if (execv(path, comando) == -1)
		perror("Comando non riconosciuto");
	exit(0);
}

void executePipe(char *buf)
{ // esegue i comandi di una pipe binaria ( comando1 | comando2 )
	if (strstr(buf, "| |") != NULL)
	{
		printf("pipe: empty command \"| |\"\n");
		return;
	}
	char *command = malloc(buf_size); //
	char *options = malloc(buf_size);
	char *path = malloc(buf_size);
	char *com = malloc(buf_size);
	char *opt = malloc(buf_size);
	char *pat = malloc(buf_size);
	command = strtok_r(buf, " ", &buf); // primo comando
	if (buf[0] == '|')
		options = "\n"; // caso in cui il primo comando non abbia opzioni
	else
		options = strtok_r(buf, "|", &buf); // opzioni del primo comando
	strcpy(path, "/bin/");
	strcat(path, command); // path del primo comando
	buf++;				   // salto lo spazio bianco
	if (strstr(buf, "<") != NULL)
	{
		printf("only the first command could have input redirection\n");
		return;
	}
	com = strtok_r(buf, " \n", &buf); // secondo comando
	opt = strcpy(opt, buf);			  // opzioni secondo comando
	strcpy(pat, "/bin/");
	strcat(pat, com); // path del secondo comando
	if (strcmp(command, "cd") == 0 || strcmp(com, "cd") == 0)
	{
		printf("cd: cd must be used alone (without pipe)\n");
		return;
	}
	pid_t wstat = 0;
	int pipe_arr[2];
	if (pipe(pipe_arr) == -1)
	{
		perror("Error:");
		return;
	}
	const pid_t p1 = fork();
	if (p1 == -1)
	{
		perror("fork");
		return;
	}
	else if (p1 == 0)
	{
		if (dup2(pipe_arr[1], 1) == -1)
		{
			perror("dup2");
			return;
		}
		if (close(pipe_arr[0]) == -1)
		{
			perror("close");
			return;
		}
		if (close(pipe_arr[1]) == -1)
		{
			perror("close");
			return;
		}
		execute(path, command, options);
		exit(0);
	}
	else
	{
		const pid_t p2 = fork();

		if (p2 == -1)
		{
			perror("fork");
			return;
		}
		else if (p2 == 0)
		{

			if (dup2(pipe_arr[0], 0) == -1)
			{
				perror("dup2");
				return;
			}
			if (close(pipe_arr[0]) == -1)
			{
				perror("close");
				return;
			}
			if (close(pipe_arr[1]) == -1)
			{
				perror("close");
				return;
			}
			execute(pat, com, opt);
			exit(0);
		}
		else
		{
			if (wait(&wstat) == -1)
			{
				perror("Wait");
				return;
			}
			if (!WIFEXITED(wstat))
			{
				printf("The process was terminated by a non-zero status: %d\n", WEXITSTATUS(wstat));
			}
			if (WIFSIGNALED(wstat))
			{
				printf("The process was terminated by a signal: %d\n", WTERMSIG(wstat));
			}
		}
	}
	if (close(pipe_arr[0]) == -1)
	{
		perror("close");
		return;
	}
	if (close(pipe_arr[1]) == -1)
	{
		perror("close");
		return;
	}
}

void proc(char *command, char *options, char *redir)
{ // lancia comandi non in pipe (esegue redirezioni se richieste)
	if (redir[1] == ' ')
	{
		printf("redirection file not specified\nthere is a ' ' after '>' or '<'\n");
		return;
	}
	command = strtok_r(NULL, "\n", &command);
	char *path = malloc(buf_size);
	strcpy(path, "/bin/");
	strcat(path, command); // concatena path e command e lo salva in path  /bin/command
	pid_t wstat = 0;
	const pid_t p = fork();
	if (p == -1)
	{
		perror("fork");
		free(path);
		return;
	}
	else if (p == 0)
	{
		if (strlen(redir) > 1)
		{
			char *File = malloc(buf_size);
			strcpy(File, redir);
			char *b = NULL;
			char *currDir = getcwd(b, 0);
			strcat(currDir, "/");
			if (File[0] == '>')
			{
				File++;
				strcat(currDir, File);
				currDir = strtok_r(NULL, "\n", &currDir);
				printf("currDir: %s\n", currDir);
				int fd = open(currDir, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
				if (fd == -1)
				{
					perror("Open");
					return;
				}
				dup2(fd, 1);
				close(fd);
			}
			else if (File[0] == '<')
			{
				File++;
				strcat(currDir, File);
				currDir = strtok_r(NULL, "\n", &currDir);
				int fd = open(currDir, O_RDONLY);
				if (fd == -1)
				{
					perror("Open");
					return;
				}
				dup2(fd, 0);
				close(fd);
			}
		}
		execute(path, command, options);
	}
	else
	{
		if (wait(&wstat) == -1)
		{
			perror("Wait");
			free(path);
			return;
		}
		if (!WIFEXITED(wstat))
		{
			printf("The process was terminated by a non-zero status: %d\n", WEXITSTATUS(wstat));
		}
		if (WIFSIGNALED(wstat))
		{
			printf("The process was terminated by a signal: %d\n", WTERMSIG(wstat));
		}
	}
	free(path);
}

void cmd_control(char *buf)
{ // buf contiene il comando da eseguire
	char *command = malloc(buf_size);
	char *options = malloc(buf_size);
	char *redir = malloc(buf_size);

	command = strtok_r(buf, " ", &buf); // metto dentro command il nome del comando
	if (buf[0] == '>' || buf[0] == '<')
	{ // controllo se c'è da fare una redirezione di I/O
		redir = strcpy(redir, buf);
		options = "\n";
	}
	else
	{
		if (strstr(buf, ">") != NULL)
		{
			options = strtok_r(NULL, ">", &buf);
			redir = strcpy(redir, ">");
			redir = strcat(redir, buf);
		}
		else if (strstr(buf, "<") != NULL)
		{
			options = strtok_r(NULL, "<", &buf);
			redir = strcpy(redir, "<");
			redir = strcat(redir, buf);
		}
		else
		{
			options = strcpy(options, buf);
			redir = "\n";
		}
	}
	// cd()
	if (strcmp(command, "cd") == 0)
	{
		if (strstr(buf, "|") != NULL)
		{ // cerca la prima occorenza di "|"
			printf("cd: error pipe\n");
			return;
		}
		if (redir[0] == '<' || redir[0] == '>')
		{
			printf("cd: error redirect\n");
			return;
		}
		if (strstr(buf, " ") != NULL)
		{
			printf("cd: Too much arguments\n");
			return;
		}
		buf = strtok_r(NULL, "\n", &buf);
		if (chdir(buf) == -1)
			fprintf(stderr, "Error: %s\n", strerror(errno));
	}
	else
	{
		proc(command, options, redir);
	}

	free(command);
	// free(options);
	// free(redir);
}

int main(int argc, char *argv[])
{
	char *buf = malloc(buf_size);
	if (buf == NULL)
	{
		fprintf(stderr, "Cannot allocate the buffer\n");
		return -1;
	}
	for (;;)
	{
		char *currDir = getcwd(NULL, 0); // store della current directory
		if (currDir == NULL)
			fprintf(stderr, "Error: %s\n", strerror(errno));
		else
		{
			usleep(100000);
			printf("%s: %s $ ", getenv("USER"), currDir);
			char *cmd = fgets(buf, buf_size, stdin); // leggo comando da stdin
			if (cmd == NULL)
			{
				printf("exit\n");
			}
			if (strncmp(cmd, "exit", 4) == 0)
				break;
			if (strcmp(cmd, "\n") != 0)
			{
				if (strstr(cmd, " | "))
					executePipe(cmd);
				else
					cmd_control(cmd);
			}
		}
	}
}