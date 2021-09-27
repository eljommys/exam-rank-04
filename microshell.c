#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

#define TYPE_END	0
#define TYPE_PIPE	1
#define TYPE_BREAK	2

typedef struct s_list
{
	int length;
	char **args;
	int pipes[2];
	int type;
	struct s_list *previous;
	struct s_list *next;
}			t_list;

int show_error(const char *str)
{
	write(2, str, strlen(str));
	return (EXIT_FAILURE);
}

int	exit_fatal()
{
	show_error("error: fatal\n");
	exit(EXIT_FAILURE);
	return (EXIT_FAILURE);
}

void *exit_fatal_ptr()
{
	show_error("error: fatal\n");
	exit(EXIT_FAILURE);
	return (NULL);
}

char *ft_strdup(const char *str)
{
	int len;
	int i;
	char *new;

	len = strlen(str);
	if (!(new = (char *)malloc(len + 1)))
		return (exit_fatal_ptr());
	i = -1;
	while (++i < len)
		new[i] = str[i];
	new[i] = 0;
	return (new);
}
int	add_arg(t_list *cmd, char *arg)
{
	char **new;
	int i;

	if (!(new = (char **)malloc(sizeof(char *) * (cmd->length + 2))))
		return (exit_fatal());
	i = -1;
	while (++i < cmd->length)
		new[i] = cmd->args[i];
	if (cmd->args)
		free(cmd->args);
	if (!(new[i++] = ft_strdup(arg)))
		return (exit_fatal());
	new[i] = 0;
	cmd->length++;
	cmd->args = new;
	return (EXIT_SUCCESS);
}

int list_add(t_list **cmd, char *arg)
{
	t_list *new;

	if (!(new = (t_list *)malloc(sizeof(t_list))))
		return (exit_fatal());
	new->length = 0;
	new->args = NULL;
	new->type = TYPE_END;
	new->previous = NULL;
	new->next = NULL;
	if (*cmd)
	{
		new->previous = *cmd;
		(*cmd)->next = new;
	}
	*cmd = new;
	return (add_arg(*cmd, arg));
}

int	parse_arg(t_list **cmd, char *arg)
{
	int is_break;

	is_break = (strcmp(";", arg) == 0);
	if (!*cmd || ((*cmd)->type != TYPE_END && !is_break))
		return (list_add(cmd, arg));
	if (!strcmp(arg, "|"))
		(*cmd)->type = TYPE_PIPE;
	else if (is_break)
		(*cmd)->type = TYPE_BREAK;
	else
		return (add_arg(*cmd, arg));
	return (EXIT_SUCCESS);
}

void	list_rewind(t_list **cmd)
{
	while ((*cmd)->previous)
		*cmd = (*cmd)->previous;
}

void	free_cmds(t_list *cmd)
{
	int i;
	t_list *aux;

	while (cmd)
	{
		i = -1;
		while (++i < cmd->length)
			free(cmd->args[i]);
		free(cmd->args);
		aux = cmd->next;
		free(cmd);
		cmd = aux;
	}
}

int	exec_cmd(t_list *cmd, char **env)
{
	int status;
	int	ret;
	pid_t pid;
	int	pipe_on;

	pipe_on = 0;
	ret = EXIT_FAILURE;
	if (cmd->type == TYPE_PIPE || (cmd->previous && cmd->previous->type == TYPE_PIPE))
	{
		if (pipe(cmd->pipes))
			return (exit_fatal());
		pipe_on = 1;
	}
	pid = fork();
	if (pid == 0)
	{
		if (cmd->type == TYPE_PIPE && dup2(cmd->pipes[1], 1) < 0)
			return (exit_fatal());
		if (cmd->previous && cmd->previous->type == TYPE_PIPE && dup2(cmd->previous->pipes[0], 0))
			return (exit_fatal());
		if ((ret = execve(cmd->args[0], cmd->args, env)) < 0)
		{
			show_error("error: cannot execute ");
			show_error(cmd->args[0]);
			show_error("\n");
		}
		exit(ret);
	}
	waitpid(pid, &status, 0);
	if (pipe_on)
	{
		close(cmd->pipes[1]);
		if (cmd->type != TYPE_PIPE)
			close(cmd->pipes[0]);
	}
	if (cmd->previous && cmd->previous->type == TYPE_PIPE)
		close(cmd->previous->pipes[0]);
	if (WIFEXITED(status))
		ret = WEXITSTATUS(status);
	return (ret);
}

int	exec_cmds(t_list *cmd, char **env)
{
	int		ret;

	ret = EXIT_SUCCESS;
	while (cmd)
	{
		if (!strcmp(cmd->args[0], "cd"))
		{
			if (cmd->length != 2)
				show_error("error: cd: bad arguments");
			else if (chdir(cmd->args[1]))
			{
				show_error("error: cd: cannot change directory to ");
				show_error(cmd->args[1]);
				show_error("\n");
			}
			else
				continue ;
			return(EXIT_FAILURE);
		}
		else
			ret = exec_cmd(cmd, env);
		cmd = cmd->next;
	}
	return (ret);
}

int main(int argc, char **argv, char **env)
{
	t_list *cmds;
	int i;

	i = 0;
	cmds = NULL;
	while (++i < argc) if (parse_arg(&cmds, argv[i]))
		return (EXIT_FAILURE);
	list_rewind(&cmds);
	i = exec_cmds(cmds, env);
	free_cmds(cmds);
	return (i);
}
