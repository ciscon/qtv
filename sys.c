/*
System dependant stuff, live hard.
Also contain some "misc" functions, have no idea where to put it, u r welcome to sort out it.
*/
 
#include "qtv.h"


#ifdef _WIN32

//FIXME: replace this shit with linux/FreeBSD code, so we will be equal on all OSes

int snprintf(char *buffer, size_t count, char const *format, ...)
{
	int ret;
	va_list argptr;
	if (!count)
		return 0;
	va_start(argptr, format);
	ret = _vsnprintf(buffer, count, format, argptr);
	buffer[count - 1] = 0;
	va_end(argptr);
	return ret;
}

int Q_vsnprintf(char *buffer, size_t count, const char *format, va_list argptr)
{
	int ret;
	if (!count)
		return 0;
	ret = _vsnprintf(buffer, count, format, argptr);
	buffer[count - 1] = 0;
	return ret;
}

#endif

#if defined(__linux__) || defined(_WIN32) || defined(__CYGWIN__)

/* 
 * Functions strlcpy, strlcat
 * was copied from FreeBSD 4.10 libc: src/lib/libc/string/
 *
 *  // VVD
 */
size_t strlcpy(char *dst, char *src, size_t siz)
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0)
	{
		do
		{
			if ((*d++ = *s++) == 0)
				break;
		}
		while (--n != 0);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0)
	{
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}

size_t strlcat(char *dst, char *src, size_t siz)
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0')
	{
		if (n != 1)
		{
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));       /* count does not include NUL */
}

#endif

static unsigned long sys_timeBase = 0;

unsigned int Sys_Milliseconds(void)
{
#ifdef _WIN32
	#ifdef _MSC_VER
		#pragma comment(lib, "winmm.lib")
	#endif

	if (!sys_timeBase)
		sys_timeBase = timeGetTime();

	return timeGetTime() - sys_timeBase;
#else
	//assume every other system follows standards.
	struct timeval tv;

	gettimeofday(&tv, NULL);

	if (!sys_timeBase)
	{
		sys_timeBase = (unsigned)tv.tv_sec;
		return ((unsigned)tv.tv_usec) / 1000;
	}

	return ((unsigned)tv.tv_sec - sys_timeBase) * 1000 + (((unsigned)tv.tv_usec) / 1000);
#endif
}

char	*redirect_buf		= NULL;
int		redirect_buf_size	= 0;

void Sys_RedirectStart(char *buf, int size)
{
	if (redirect_buf) {
		Sys_RedirectStop();
		Sys_Printf(NULL, "BUG: second Sys_RedirectStart() without Sys_RedirectStop()\n");
	}
	
	if (size < 1 || !buf)
		return; // nigga plz

	redirect_buf		= buf;
	redirect_buf[0]		= 0;
	redirect_buf_size	= size;
}

void Sys_RedirectStop(void)
{
	if (!redirect_buf)
		Sys_Printf(NULL, "BUG: Sys_RedirectStop() without Sys_RedirectStart()\n");

	redirect_buf		= NULL;
	redirect_buf_size	= 0;
}

void Sys_Printf(cluster_t *cluster, char *fmt, ...)
{
	va_list		argptr;
	char		string[2048];
	unsigned char *t;
	
	va_start (argptr, fmt);
	Q_vsnprintf (string, sizeof(string), fmt, argptr);
	va_end (argptr);

	// cluster may be NULL, if someday u start use it...
	// some Sys_Printf used as Sys_Printf (NULL, "lalala\n"); so...

	for (t = (unsigned char*)string; *t; t++)
	{
		if (*t >= 146 && *t < 156)
			*t = *t - 146 + '0';
		if (*t == 143)
			*t = '.';
		if (*t == 157 || *t == 158 || *t == 159)
			*t = '-';
		if (*t >= 128)
			*t -= 128;
		if (*t == 16)
			*t = '[';
		if (*t == 17)
			*t = ']';
		if (*t == 29)
			*t = '-';
		if (*t == 30)
			*t = '-';
		if (*t == 31)
			*t = '-';
		if (*t == '\a')	//doh. :D
			*t = ' ';
	}

	if (redirect_buf && redirect_buf_size > 0)
		strlcat(redirect_buf, string, redirect_buf_size);

	printf("%s", string);
}

// print debug
// this is just wrapper for Sys_Printf(), but print nothing if developer 0
void Sys_DPrintf(cluster_t *cluster, char *fmt, ...)
{
	va_list		argptr;
	char		string[2048];
	
	if (!developer.integer)
		return;
	
	va_start (argptr, fmt);
	Q_vsnprintf (string, sizeof(string), fmt, argptr);
	va_end (argptr);

	Sys_Printf(cluster, "%s", string);
}


void Sys_Exit(int code)
{
	exit(code);
}

void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[2048];

	va_start (argptr, error);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	strlcat(text, "\n", sizeof(text));
	Sys_Printf(NULL, text);

	Sys_Exit (1);
}


/*
===================
Sys_malloc

Use it instead of malloc so that if memory allocation fails,
the program exits with a message saying there's not enough memory
instead of crashing after trying to use a NULL pointer.
It also sets memory to zero.
===================
*/
void *Sys_malloc (size_t size)
{
	void *p = malloc(size);

	if (!p)
		Sys_Error ("Sys_malloc: Not enough memory free");
	memset(p, 0, size);

	return p;
}

char *Sys_strdup (const char *src)
{
	char *p = strdup(src);

	if (!p)
		Sys_Error ("Sys_strdup: Not enough memory free");

	return p;
}

// return file extension with dot, or empty string if dot not found at all
const char *Sys_FileExtension (const char *in)
{
	const char *out = strrchr(in, '.');

	return ( out ? out : "" );
}

// absolute paths are prohibited
qbool Sys_SafePath(const char *in)
{
	return ( (in[0] == '\\' || in[0] == '/' || strstr(in, "..") || (in[0] && in[1] == ':')) ? false : true );
}

// use it on file which open in BINARY mode
int Sys_FileLength (FILE *f)
{
	int		pos;
	int		end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

// open file in BINARY mode and return file size, if open failed then -1 returned
int Sys_FileOpenRead (const char *path, FILE **hndl)
{
	FILE *f;
	int size;

	*hndl = NULL;

	if (!(f = fopen(path, "rb")))
		return -1;

	if ((size = Sys_FileLength(f)) < 0) {
		Sys_Printf(NULL, "Sys_FileOpenRead: bug\n");
		fclose(f); // well, we open file, but size negative, close file and return -1
		return -1;
	}

	*hndl = f;

	return size;
}


// qqshka - hmm, seems in C this is macroses, i don't like macroses,
// however this functions work wrong on unsigned types!!!

#ifdef KTX_MIN

double min( double a, double b )
{
	return ( a < b ? a : b );
}

#endif

#ifdef KTX_MAX

double max( double a, double b )
{
	return ( a > b ? a : b );
}

#endif

double bound( double a, double b, double c )
{
	return ( a >= c ? a : b < a ? a : b > c ? c : b);
}

// handle keyboard input
void Sys_ReadSTDIN(cluster_t *cluster, fd_set socketset)
{
#ifdef _WIN32

	for (;;)
	{
		char c;

		if (!_kbhit())
			break;
		c = _getch();

		if (c == '\n' || c == '\r')
		{
			Sys_Printf(cluster, "\n");
			if (cluster->inputlength)
			{
				cluster->commandinput[cluster->inputlength] = '\0';
				Cbuf_InsertText(cluster->commandinput);

				cluster->inputlength = 0;
				cluster->commandinput[0] = '\0';
			}
		}
		else if (c == '\b')
		{
			if (cluster->inputlength > 0)
			{
				Sys_Printf(cluster, "%c", c);
				Sys_Printf(cluster, " ", c);
				Sys_Printf(cluster, "%c", c);

				cluster->inputlength--;
				cluster->commandinput[cluster->inputlength] = '\0';
			}
		}
		else
		{
			Sys_Printf(cluster, "%c", c);
			if (cluster->inputlength < sizeof(cluster->commandinput)-1)
			{
				cluster->commandinput[cluster->inputlength++] = c;
				cluster->commandinput[cluster->inputlength] = '\0';
			}
		}
	}

#else

	if (FD_ISSET(STDIN, &socketset))
	{
		cluster->inputlength = read (STDIN, cluster->commandinput, sizeof(cluster->commandinput));
		if (cluster->inputlength >= 1)
		{
			cluster->commandinput[cluster->inputlength-1] = 0;        // rip off the /n and terminate
			cluster->inputlength--;

			if (cluster->inputlength)
			{
				cluster->commandinput[cluster->inputlength] = '\0';
				Cbuf_InsertText(cluster->commandinput);

				cluster->inputlength = 0;
				cluster->commandinput[0] = '\0';
			}
		}
	}

#endif
}

/*==================
Sys_ReplaceChar
Replace char in string
==================*/
void Sys_ReplaceChar(char *s, char from, char to)
{
	if (s)
		for ( ;*s ; ++s)
			if (*s == from)
				*s = to;
}

unsigned long Sys_HashKey (const char *str)
{
	unsigned long hash = 0;
	int c;

	// the (c&~32) makes it case-insensitive
	// hash function known as sdbm, used in gawk
	while ((c = *str++))
        hash = (c &~ 32) + (hash << 6) + (hash << 16) - hash;

    return hash;
}
