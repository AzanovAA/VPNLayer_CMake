#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
	if (argc != 2)
		return printf("Usage: ./stop uuid\n"), -1;

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

				str[J] = 'X'; // do not pass suspicious values to executed program
		}	}
	} /* }}} */

	setuid(0);
	setgid(0);

	char buf[1024];

    strcpy(buf,"sh /var/run/vpnlayer");
	strncat(buf,"/",1);
	strncat(buf,argv[1],128);
	strncat(buf,"/stop.sh",8);
	system(buf);

    strcpy(buf,"/var/run/vpnlayer");
	strncat(buf,"/",1);
	strncat(buf,argv[1],128);
	strncat(buf,"/stop.sh",8);
	unlink(buf);

    strcpy(buf,"/var/run/vpnlayer");
	strncat(buf,"/",1);
	strncat(buf,argv[1],128);
	strncat(buf,"/config",7);
	unlink(buf);

    strcpy(buf,"/var/run/vpnlayer");
	strncat(buf,"/",1);
	strncat(buf,argv[1],128);
	rmdir(buf);

	return 0;
}
