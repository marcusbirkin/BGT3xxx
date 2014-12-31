#include <asm/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>

#include "input_keynames.h"


static
void print_error (const char *action, const char *file)
__attribute__ ((noreturn));


static
void print_error (const char *action, const char *file)
{
	static const char msg [] = "\nERROR: could not ";

	write (0, msg, strlen(msg));
	write (0, action, strlen(action));
	write (0, " '", 2);
	write (0, file, strlen(file));
	write (0, "'\n\n", 3);
	exit (-1);
}


static
int parse_keyname (char *pos, char **nend, int limit)
{
	int cmp, _index;
	int l = 1;
	const struct input_key_name *kn;
	int r;

	if (limit < 5)
		return -1;

	while ((*pos == ' ' || *pos == '\t') && limit > 0) {
		(*nend)++;
		pos++;
		limit--;
	}

	if (pos[3] != '_')
		return -2;

	if (pos[0] == 'K' && pos[1] == 'E' && pos[2] == 'Y') {
		kn = key_name;
		r = sizeof (key_name) / sizeof (key_name[0]);
	}
	else if (pos[0] == 'B' && pos[1] == 'T' && pos[2] == 'N') {
		kn = btn_name;
		r = sizeof (btn_name) / sizeof (btn_name[0]);
	}
	else
		return -2;

	(*nend) += 4;
	pos += 4;
	limit -= 4;

	while (r >= l) {
		int len0, len1 = 0;

		_index = (l + r) / 2;

		len0 = strlen(kn[_index-1].name);

		while (len1 < limit && isgraph(pos[len1]))
			len1++;

		cmp = strncmp (kn[_index-1].name, pos,
			       strlen(kn[_index-1].name));

		if (len0 < len1 && cmp == 0)
			cmp = -1;

		if (cmp == 0) {
			*nend = pos + strlen (kn[_index-1].name);

			if (**nend != '\n' &&
			    **nend != '\t' &&
			    **nend != ' ' &&
			    *nend != pos)
				return -3;

			return kn[_index-1].key;
		}

		if (cmp < 0)
			l = _index + 1;
		else
			r = _index - 1;

		if (r < l) {
			static const char msg [] = "\nunknown key '";
			write (0, msg, strlen(msg));
			write (0, pos-4, len1 + 4);
			write (0, "'\n", 2);
		}
	};

	return -4;
}



const char usage [] = "\n\tusage: av7110_loadkeys [-i|--invert] [-a|--address <num>] keymap_filename.(rc5|rcmm)\n\n";


struct ir_setup {
	__u32 ir_config;
	__u16 keytab [256];
} __attribute__ ((packed));


int main (int argc, char **argv)
{
	static struct ir_setup setup;
	int i, fd;
	size_t len;
	char *buf, *pos, *fname = NULL;

	for (i=1; i<argc; i++) {
		if (!strcmp("-i", argv[i]) || !strcmp("--invert", argv[i]))
			setup.ir_config |= 0x8000;
		else if (!strcmp("-a", argv[i]) || !strcmp("--address", argv[i])) {
			if (++i < argc) {
				setup.ir_config |= (atoi(argv[i]) & 0xff) << 16;
				setup.ir_config |= 0x4000;
			}
		} else
			fname = argv[i];
	}

	if (!fname) {
		write (0, usage, strlen(usage));
		exit (-1);
	}

	if (strncmp(".rcmm", fname + strlen(fname) - 5, 5) == 0)
		setup.ir_config |= 0x0001;
	else if (strncmp(".rc5", fname + strlen(fname) - 4, 4) != 0) {
		const char msg [] = "\nERROR: "
			"input filename must have suffix .rc5 or .rcmm\n";
		write (0, msg, strlen(msg));
		exit (-1);
	}

	if ((fd = open (fname, O_RDONLY)) < 0)
		print_error ("open", fname);

	len = lseek (fd, 0, SEEK_END);

	if (!(pos = buf = mmap (NULL, len, PROT_READ, MAP_PRIVATE, fd, 0)))
		print_error ("mmap", fname);

	while (pos < buf + len) {
		int key, keycode;

		while (!isxdigit(*pos) && pos < buf + len)
			pos++;

		if (pos == buf + len)
			break;

		key = strtol (pos, &pos, 0);
		keycode = parse_keyname (pos, &pos, buf + len - pos);

		if (key < 0 || key > 0xff) {
			const char msg [] =
				"\nERROR: key must be in range 0 ... 0xff!\n\n";

			write (0, msg, strlen(msg));
			exit (-1);
		}

		if (keycode < 0)
			print_error ("parse", fname);

		setup.keytab[key] = keycode;
	}

	munmap (buf, len);
	close (fd);

	write (1, &setup, 4 + 256 * sizeof(__u16));

	return 0;
}
