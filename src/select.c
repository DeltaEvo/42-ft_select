/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   select.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dde-jesu <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/02/08 13:29:36 by dde-jesu          #+#    #+#             */
/*   Updated: 2019/02/23 12:41:52 by dde-jesu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <term.h>
#include <unistd.h>
#include <string.h>
#include "ft/str.h"

#define NO_TERM "TERM is not defined\n"
#define NO_TERMCAPS "TERMCAPS database not accessible\n"
#define UNKNOWN_TERM "TERM unknown\n"

enum	e_arg_type {
	ARG_NORMAL,
	ARG_FILE,
	ARG_DIRECTORY
};

struct	s_arg {
	enum e_arg_type	type;
	char			*value;
	size_t			len;
	bool			selected;
};

struct	s_select {
	struct s_arg	*args;
	size_t			args_size;
	size_t			max_len;
	size_t			cursor;

	struct winsize	win_size;

	char			*tc_goto;
	char			*tc_clear;
};

struct winsize get_win_size()
{
	struct winsize w;

	// TODO HANDLE ERROR
	ioctl(0, TIOCGWINSZ, &w);

	return (w);
}

bool	switch_to_raw(int fd, struct termios *orig_termios)
{
	struct termios	raw;

	if (tcgetattr(fd, orig_termios) == -1)
		return (false);
	raw = *orig_termios;
	raw.c_lflag &= ~(ECHO | ICANON);
	if (tcsetattr(fd, TCSAFLUSH, &raw) < 0)
		return (false);
	return (true);
}

bool	setup_term(struct termios *orig)
{
	char	*term;
	int		success;

	if (!(term = getenv("TERM")))
	{
		write(2, NO_TERM, sizeof(NO_TERM) - 1);
		return (false);
	}

	if ((success = tgetent(NULL, term)) < 0)
	{
		write(2, NO_TERMCAPS, sizeof(NO_TERMCAPS) - 1);
		return (false);
	}
	else if (success == 0)
	{
		write(2, UNKNOWN_TERM, sizeof(UNKNOWN_TERM) - 1);
		return (false);
	}
	return (switch_to_raw(STDIN_FILENO, orig));
}

#include <sys/stat.h>

enum e_arg_type	get_arg_type(char *arg)
{
	struct stat info;

	if (stat(arg, &info) == -1)
		return (ARG_NORMAL);
	if (S_ISDIR(info.st_mode))
		return (ARG_DIRECTORY);
	return (ARG_FILE);

}

struct s_arg	*create_args(int argc, char *argv[], size_t *max_len)
{
	struct s_arg	*args;
	int				i;

	if (!(args = malloc(argc * sizeof(*args))))
		return (NULL);
	i = 0;
	while (i < argc)
	{
		args[i] = (struct s_arg) {
			.type = get_arg_type(argv[i]),
			.value = argv[i],
			.len = ft_strlen(argv[i])
		};
		if (args[i].len > *max_len)
			*max_len = args[i].len;
		i++;
	}
	return (args);
}

#include <unistd.h>

int	writechar(int c)
{
	ssize_t	ret;

	ret = write(STDOUT_FILENO, &c, 1);
	return (ret < 0 ? -1 : c);
}


#define CSI "\33["

/*
** [ + space + name + space + ] + space
*/
#define EXTRA_SPACE 5

void	render(struct s_select *select)
{
	size_t	i;
	size_t	x;
	size_t	y;
	size_t	cols;

	tputs(select->tc_clear, 1, writechar);
	i = 0;
	y = 0;
	cols = select->max_len + EXTRA_SPACE;
	if (select->args_size < select->win_size.ws_col / (select->max_len + 1))
		cols = select->win_size.ws_col / select->args_size;
	while (i < select->args_size)
	{
		x = 0;
		while (x < select->win_size.ws_col / (select->max_len + EXTRA_SPACE) && i < select->args_size)
		{
			tputs(tgoto(select->tc_goto, x * cols, y), 1, writechar);
			if (select->cursor == i)
				write(STDOUT_FILENO, "[ ", 2);
			else
				write(STDOUT_FILENO, "  ", 2);
			if (select->args[i].selected)
				write(STDOUT_FILENO, CSI "7m", sizeof(CSI) + 1);
			if (select->args[i].type == ARG_DIRECTORY)
				write(STDOUT_FILENO, CSI "4;96m", sizeof(CSI) + 4);
			else if (select->args[i].type == ARG_FILE)
				write(STDOUT_FILENO, CSI "91m", sizeof(CSI) + 2);
			else if (select->args[i].type == ARG_NORMAL)
				write(STDOUT_FILENO, CSI "97m", sizeof(CSI) + 2);
			write(STDOUT_FILENO, select->args[i].value, select->args[i].len);
			write(STDOUT_FILENO, CSI "0m", sizeof(CSI) + 1);
			if (select->cursor == i)
				write(STDOUT_FILENO, " ]", 2);
			else
				write(STDOUT_FILENO, "  ", 2);
			i++;
			x++;
		}
		y++;
	}
}

#define TC_ENABLE_ALTERNATE_BUFFER "ti"
#define TC_DISABLE_ALTERNATE_BUFFER "te"
#define TC_INVISIBLE_CURSOR "vi"
#define TC_VISIBLE_CURSOR "ve"
#define TC_GOTO "cm"
#define TC_CLEAR "cl"

int	main(int argc, char *argv[])
{
	struct s_select	select;
	struct termios	oterm;
	char c;

	if (!setup_term(&oterm))
		return (1);

	select.win_size = get_win_size();
	select.max_len = 0;
	select.cursor = 0;
	select.tc_goto = tgetstr(TC_GOTO, NULL);
	select.tc_clear = tgetstr(TC_CLEAR, NULL);
	select.args_size = argc - 1;
	select.args = create_args(select.args_size, argv + 1, &select.max_len);
	tputs(tgetstr(TC_ENABLE_ALTERNATE_BUFFER, NULL), 1, writechar);
	tputs(tgetstr(TC_INVISIBLE_CURSOR, NULL), 1, writechar);
	render(&select);

	while (read(STDIN_FILENO, &c, 1) == 1)
	{
		if (c == ' ')
			select.args[select.cursor].selected = true;
		if (c == 'j')
			select.cursor--;
		if (c == 'k')
			select.cursor++;
		render(&select);
	}

	tputs(tgetstr(TC_DISABLE_ALTERNATE_BUFFER, NULL), 1, writechar);
	tputs(tgetstr(TC_VISIBLE_CURSOR, NULL), 1, writechar);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &oterm);
}
