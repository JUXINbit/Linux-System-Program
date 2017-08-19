#define _CRT_SECURE_NO_WARNINGS 1

#include<stdio.h>
#include<string.h>
#include<signal.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/utsname.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<sys/ptrace.h>

#define MAX 1024
#define MAX_COMM 100

void print_prompt();
void bg_struct_handle(pid_t pid, char *arg[], int type);
void file_out(char *arg[], char *out_file, int type);
char cwd[MAX];//���浱ǰ·��
char *all[MAX];//�����������ַ���
int current_out = 1;
int current_in = 0;
int fd[4];
/*

*�����̨����

*/
typedef struct proc{
	pid_t pid;
	int status;
	char *arg[MAX_COMM];
	struct proc *next;
}proc;

proc *start;

void bg_signal_handle()
{
	int status;
	pid_t pid;
	pid = waitpid(-1, &status, WNOHANG);//������
	proc * iterate;
	iterate = start;
	while (iterate != NULL)
	{
		if (iterate->pid == getpid())
		{
			bg_struct_handle(pid, NULL, 1);
		}
		iterate = iterate->next;
	}
}

//type : 0 insert 
void bg_struct_handle(pid_t pid, char *arg[], int type)
{
	proc *iterate, *new;
	if (type == 0)
	{
		if (start == NULL)
		{
			start = (proc *)malloc(sizeof(proc));
			start->pid = pid;
			start->status = 1;
			start->next = NULL;
			int i = 0;
			while (arg[i] != NULL)
			{
				start->arg[i] = malloc(MAX_COMM*sizeof(char));
				strcpy(start->arg[i], arg[i]);
				i += 1;
			}
			start->arg[i] = NULL;
		}
		else
		{
			new = (proc *)malloc(sizeof(proc));
			new->pid = pid;
			new->status = 1;
			new->next = NULL;
			int i = 0;
			while (arg[i] != NULL)
			{
				new->arg[i] = malloc(MAX_COMM *sizeof(char));
				strcpy(new->arg[i], arg[i]);
				i += 1;
			}
			new->arg[i] = NULL;
			iterate = start;
			while (iterate->next != NULL)
				iterate = iterate->next;
			iterate->next = new;
		}
	}
	else if (type == 1)
	{
		proc *preite = NULL;
		iterate = start;
		while (iterate != NULL&&iterate->pid != pid)
		{
			preite = iterate;
			iterate = iterate->next;
		}
		if (iterate == NULL)
		{
			printf("NO such pid\n");
			return;
		}
		else if (iterate->pid == pid)
		{
			if (preite == NULL)
			{
				start = iterate->next;
				free(iterate);
			}
			else
			{
			   preite->next = iterate->next;
			   free(iterate);
			}
		}
	}
	else if (type == 2)
	{
		int i = 1, a = 0;
		iterate = start;
		if (iterate == NULL)
		{
			printf("NO Background jobs\n");
			return;
		}
		while (iterate != NULL)
		{
			a = 0;
			setbuf(stdout, NULL);
			printf("[%d] ", i);
			while (iterate->arg[a] != NULL)
			{
				printf("%s ", iterate->arg[a]);
				a += 1;
			}
			printf("[%d]\n", iterate->pid);
			i += 1;
			iterate = iterate->next;
		}
	}
	return;
}
void print_prompt()
{
	struct utsname uname_ptr;//��ȡϵͳ��Ϣ
	uname(&uname_ptr);
	getcwd(cwd, sizeof(cwd));
	setbuf(stdout, NULL);//ֱ���������
	printf("<%s@%s:%s>", uname_ptr.nodename, uname_ptr.sysname, cwd);
}
void scan_command(char *command)
{
	int bytes_read;
	size_t nbytes = MAX;
	bytes_read = getline(&command, &nbytes, stdin);
	bytes_read -= 1;
	command[bytes_read] = '\0';
}
void *parse(char* command, int time)
{
	char*comm;
	if (time == 0)
		comm = strtok(command, " ");
	else
	comm = strtok(NULL, " ");
	return comm;
}

/*�ָ��û��Էֺŷָ�������*/

void parse_semicolon(char *command)
{
	int i;
	for (i = 0; i<MAX; i++)
	{
		all[i] = (char*)malloc(MAX_COMM *sizeof(char));
	}
	i = 0;
	all[i] = strtok(command, ";");//strtok
	while (1)
	{
		i += 1;
		all[i] = strtok(NULL, ";");
		if (all[i] == NULL)
			break;
	}
}

void cd(char *arg)
{//�ڽ�����
	if (arg == NULL)
	{
		printf("insufficient arguments\n");
	}
	else
	{
		int cond;
		cond = chdir(arg);
		if (cond == -1)
		{
			printf("wrong path\n");
		}
	}
}
void sig_handle(int sig)
{
	if (sig == 2)
	{
		printf("\nInstead of Ctrl-C type quit\n");
		print_prompt();
	}
	else if (sig == 3)
	{
		printf("\nType quit to exit\n");
		print_prompt();
	}
	signal(sig, sig_handle);
}
void bf_exec(char *arg[], int type)
{
	pid_t pid;
	if (type == 0)//ǰ̨
	{
		if ((pid = fork())<0)
		{
	      perror("fork");
			return;
		}
		else if (pid == 0)//child
		{
			signal(SIGTSTP, SIG_DFL);//��ͣ����
			execvp(arg[0], arg);//�滻����Ҫִ�е�����
		}
		else
		{
			pid_t c;
			signal(SIGTSTP, SIG_DFL);//��������
			c = wait(&pid);//������
			dup2(current_out, 1);//��ԭ��׼���
			dup2(current_in, 0);
			return;
		}
	}
	else //hou tai
	{
		signal(SIGCHLD, bg_signal_handle);
		if ((pid = fork())< 0)
		{
			perror("fork");
			return;
		}
		else if (pid == 0)
		{
			int f;
			execvp(arg[0], arg);
		}
		else
		{
			bg_struct_handle(pid, arg, 0);
			dup2(current_out, 1);
			dup2(current_in, 0);
			return;
		}
	}
}

void execute(char *command)
{
	char *arg[MAX_COMM];
	char * try;
	arg[0] = parse(command, 0);//��ȡ��������
	int t = 1;
	arg[t] = NULL;
	if (strcmp(arg[0], "cd") == 0)//����cd(�ڽ�)
	{
		try = parse(command, 1);
		cd(try);
		return;
	}
	if (strcmp(arg[0], "exit") == 0)//exit
	{
		exit(0);
	}
	while (1)
	{
		try = parse(command, 1);//����
		if (try == NULL)
		{
			break;
		}
		else if (strcmp(try, ">") == 0)//�ض���һ���ļ�
		{
			try = parse(command, 1);//�ļ���
			file_out(arg, try, 0);//0 fugai chongdingxinag
			return;
		}
		else if (strcmp(try, ">>") == 0)
		{
			try = parse(command, 1);
			printf("%s\n", try);
			file_out(arg, try, 1);//1 zhuijia chongdingxiang
			return;
		}
		else if (strcmp(try, "<") == 0)//�����ض���
		{
		}
		else if (strcmp(try, "&") == 0)//��ִ̨��
		{
			bf_exec(arg, 1);//1 hou tai
			return;
		}
		else//��ͨ�������
		{
			arg[t] = try;
			t += 1;
			arg[t] = NULL;
		}
	}
	bf_exec(arg, 0);//0 qiantai
}

void file_out(char *arg[], char *out_file, int type)

{
	int f;
	current_out = dup(1);
	if (type == 0)
	{
		f = open(out_file, O_WRONLY | O_CREAT, 0777);
		dup2(f, 1);
		close(f);
		bf_exec(arg, 0);
	}
	else
	{
		f = open(out_file, O_WRONLY | O_CREAT | O_APPEND, 0777);
		dup2(f, 1);
		close(f);
		bf_exec(arg, 0);
	}
	return;
}



void file_in(char * arg[], char *in_file, char *out_file, int type)
{
	int in;
	in = open(in_file, O_RDONLY);
	current_in = dup(0);
	dup2(in, 0);
	close(in);
	if (type == 0)
    {
		printf("Going to execute bf_exec\n");
		bf_exec(arg, 0);
	}
	else if (type == 1)
	{
  	    file_out(arg, out_file, 0);
	}
	else
	{
		file_out(arg, out_file, 1);
	}
	return;
}

int main()
{
	char *command;//qw
	int iter = 0;
	command = (char*)malloc(MAX + 1);//�洢�������
	chdir("/home/hrf");//ͨ�����������ļ�/etc/passwd
	while (1)
	{
		iter = 0;
		signal(SIGINT, sig_handle);//ctrl c
		signal(SIGQUIT, sig_handle);/*ctrl \ */
		signal(SIGCHLD, sig_handle);
		signal(SIGTSTP, SIG_IGN);//ctrl d
		//���������ʾ��
		print_prompt();
		//ɨ������������
		scan_command(command);
	//���ڷֺŽ�������������䣬������ȫ�ֱ��� all �С�
		parse_semicolon(command);
		//����ִ�е����������
		while (all[iter] != NULL)
		{
			execute(all[iter]);//���ĺ���
			iter += 1;
		}
	}
	return 0;
}