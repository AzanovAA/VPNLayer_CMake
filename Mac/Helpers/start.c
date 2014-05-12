#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <mach-o/dyld.h>

int main(int argc, char *argv[])
{
    if (argc != 6) // params: uuid protocol server port configfile
		return printf("Usage: ./start uuid protocol server port configfile\n"), -1;

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


    char username[1024], password[1024], binfile[1024], rundir[1024];
	{ // read username and password from stdin /* {{{ */
		printf("Username : "); fgets(username, sizeof(username), stdin);
		printf("Password : "); fgets(password, sizeof(password), stdin);
		username[strlen(username)-1] = '\0';
		password[strlen(password)-1] = '\0';
		printf("Username %s, password %s\n", username,password);
	} /* }}} */

    { // prepare environment /* {{{ */
		setuid(0);
		setgid(0);
		setpgid(getpid(), 0); // own process group so everything dies together

		int sz = sizeof(binfile);
		_NSGetExecutablePath(binfile, &sz);
		chdir( dirname(binfile) );

		if (0 == strcmp(argv[2], "openvpn") || 0 == strcmp(argv[2], "vpnc"))
		{
            system("/sbin/kextload ../Kext/tun.kext");
			system("/sbin/kextload ../Kext/tun-20090913.kext");
			system("/sbin/kextload ../Kext/tun-signed.kext");

            system("/sbin/kextload ../Kext/tap.kext");
			system("/sbin/kextload ../Kext/tap-20090913.kext");
			system("/sbin/kextload ../Kext/tap-signed.kext");
		}

        strcpy(rundir,"/var/run/vpnlayer");
		mkdir(rundir, 0700);

		strncat(rundir,"/",1);
		strncat(rundir,argv[1],128);
		mkdir(rundir, 0700);

		char buf[1024];
		strcpy(buf, rundir);
		strncat(buf,"/stop.sh",8);

		FILE *fstop;
		if (fstop = fopen(buf, "w"))
		{
			if (0 == strcmp(argv[2], "ssh"))
			{
				fprintf(fstop, "ps ax "
					" | sed -e 's/^ *//'"
					" | grep StrictHostKeyChecking "
					" | grep UserKnownHostsFile "
					" | grep '%s' "
					" | grep '%s' "
					" | grep '%s' "
					" | grep '%s' "
					" | grep -v 'sshpass' "
					" | cut -d' ' -f1 | xargs -n1 kill"
				, argv[3], argv[4], argv[5], username );
			}
			else
			{
				fprintf(fstop, "kill %d", getpid());
			}
			fclose(fstop);
		}

		strcpy(buf,rundir);
		strncat(buf,"/config",7);

        FILE *fconf;
		if (fconf = fopen(buf, "w"))
		{
			if (0 == strcmp(argv[2], "openvpn"))
			{
				fprintf(fconf, "%s\n%s\n", username, password);
			}
			else
			if (0 == strcmp(argv[2], "vpnc"))
			{
				fprintf( fconf , "IPSec gateway %s\n"  , argv[3]  );
				fprintf( fconf , "IPSec ID %s\n"       , argv[5]  );
				fprintf( fconf , "IPSec secret %s\n"   , argv[5]  );
				fprintf( fconf , "Local port %d\n"     , 0        );
				fprintf( fconf , "Xauth username %s\n" , username );
				fprintf( fconf , "Xauth password %s\n" , password );
			}
			fclose(fconf);
        }
	} /* }}} */

     if (0 == strcmp(argv[2], "openvpn"))
	{
		char configPath[1024];
		strcpy(configPath,rundir);
		strncat(configPath,"/config",7);

		execl("openvpn" , "openvpn"
            //, "--remote" , argv[3] , argv[4]
			, "--config" , argv[5]
            , "--auth-user-pass" , configPath
			, NULL );
	}
	else
	if (0 == strcmp(argv[2], "ssh"))
	{
		execl( "sshpass" , "sshpass" , "-p" , password , "ssh"
			, "-oStrictHostKeyChecking=no"
			, "-oUserKnownHostsFile=/dev/null"
			, "-C2TNv"
			, "-l" , username
			, "-p" , argv[4] // remote port
			, "-D" , argv[5] // local port
			, argv[3] // remote host
			, NULL );
	}
	else
	if (0 == strcmp(argv[2], "vpnc"))
	{
		char configPath[1024];
		strcpy(configPath,rundir);
		strncat(configPath,"/config",7);

		char scriptPath[1024];
		strcpy(scriptPath, dirname(binfile));
		strcat(scriptPath, "/vpnc-script");

		execl("vpnc" , "vpnc"
			, "--script" , scriptPath
			, "--no-detach"
			, configPath
			, NULL );
	}
}
