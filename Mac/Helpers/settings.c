#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <mach-o/dyld.h>

int main(int argc, char *argv[])
{
	{ // sanitize args and environment /* {{{ */
		extern char **environ;
		*environ = NULL;
		setenv("PATH","/usr/bin:/bin:/usr/sbin/:/sbin");

		int I,J;
		for (I = 0; I < argc; ++I)
		{
			char *str = argv[I];
			for (J = 0; str[J]; ++J)
			{
				if ( isalpha(str[J]) ) continue;
				if ( isdigit(str[J]) ) continue;
				if ( str[J] == '.'   ) continue;
				if ( str[J] == '_'   ) continue;
				if ( str[J] == '-'   ) continue;
				if ( str[J] == ':'   ) continue;
				if ( str[J] == ' '   ) continue;
				if ( str[J] == '('   ) continue;
				if ( str[J] == ')'   ) continue;

				str[J] = 'X'; // do not pass suspicious values to executed program
		}	}
	} /* }}} */

	setuid(0);
	setgid(0);

	char binfile[1024];
	int sz = sizeof(binfile);
	_NSGetExecutablePath(binfile, &sz);
	chdir( dirname(binfile) );

	char buf[1024];
	switch(argc)
	{
		case 2:
			sprintf( buf , "./settings.sh '%s'"
			       , argv[1]);
			system(buf);
			break;
		case 3:
			sprintf( buf , "./settings.sh '%s' '%s'"
			       , argv[1], argv[2]);
			system(buf);
			break;
		case 4:
			sprintf( buf , "./settings.sh '%s' '%s' '%s'"
			       , argv[1], argv[2], argv[3]);
			system(buf);
			break;
		case 5:
            sprintf( buf , "./settings.sh '%s' '%s' '%s' '%s'"
			       , argv[1], argv[2], argv[3], argv[4] );
			system(buf);
			break;
		case 6:
            sprintf( buf , "./settings.sh '%s' '%s' '%s' '%s' '%s'"
			       , argv[1], argv[2], argv[3], argv[4], argv[5] );
			system(buf);
			break;
    }

	return 0;
}
