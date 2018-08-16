/* Copyright (C) 2013 Webyog Inc

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA

*/


#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ClientMySQLWrapper.h"
#include "MySQLVersionHelper.h"
#include "wyString.h"
#include "CommonHelper.h"


#ifdef _WIN32
	#include <mysql.h>
#else
	#include <mysql/mysql.h>
#endif


const wyChar*
IsNewMySQL(Tunnel * tunnel, PMYSQL mysql)
{
    wyString        ver;
	const wyChar	*major, *minor, *servinfo;;
	wyChar		    seps[] = ".";
	wyInt32		    majorver, minorver;

    servinfo = sja_mysql_get_server_info(tunnel, *mysql);
    ver.SetAs(servinfo);

	major = strtok((wyChar*)ver.GetString(), seps);
	minor = strtok(NULL, seps);

	majorver = atoi(major);
	minorver = atoi(minor);

	if((majorver > 3)||(majorver == 3 && minorver > 22))
		return "`";
	else 
		return "";	
}

wyInt32	GetVersionNo(Tunnel * tunnel, PMYSQL mysql)
{
	wyString ver, errorlog;
	wyChar	 *major, *minor, *minorminor;
	wyChar	 seps[] = ".";
	wyInt32  majorver = 0, minorver = 0, minorminorver = 0;
	wyUInt32 verno = 0;

	if(!tunnel)
	{
		return verno;
	}

	ver.SetAs(sja_mysql_get_server_info(tunnel, *mysql));

	major = strtok((wyChar*)ver.GetString(), seps);
	minor = strtok(NULL, seps);
	minorminor = strtok	(NULL, seps);

	if(major)
	{
		majorver = atoi(major);
	}
	else
	{
		errorlog.Sprintf("GetVersionNo() major is NULL version = %s", ver.GetString());
		YOGLOG(0, errorlog.GetString());
	}

	if(minor)
	{
		minorver = atoi(minor);
	}
	else
	{
		errorlog.Sprintf("GetVersionNo() minor is NULL version = %s", ver.GetString());
		YOGLOG(0, errorlog.GetString());
	}

	if(minorminor)
	{
		minorminorver = atoi(minorminor);
	}
	else
	{
		errorlog.Sprintf("GetVersionNo() minorminorver is NULL version = %s", ver.GetString());
		YOGLOG(0, errorlog.GetString());
	}

	verno = majorver * 10000 + minorver * 100 + minorminorver * 1;
	
	return verno;
}

wyBool
IsMySQL4(Tunnel * tunnel, PMYSQL mysql)
{
    wyUInt32 verno = GetVersionNo(tunnel, mysql);

    if(verno >= 40000)
        return wyTrue;

    return wyFalse;
}

wyBool
IsMySQL41(Tunnel * tunnel, PMYSQL mysql)
{
	wyUInt32 me = mysql_get_server_version(*mysql);/* Only available from MySQLv4.1*/

	if(me >= 40100)
		return wyTrue;
	else
		return wyFalse;
}

wyBool
IsMySQL402(Tunnel * tunnel, PMYSQL mysql)
{
    wyString    ver;
	wyChar	    *major, *minor, *minorminor;
	wyChar	    seps[] = ".";
	wyInt32 	majorver, minorver, minorminorver;

	ver.SetAs(sja_mysql_get_server_info(tunnel, *mysql));

	major = strtok((wyChar*)ver.GetString(), seps);
	minor = strtok(NULL, seps);
	minorminor = strtok	(NULL, seps);

	majorver = atoi(major);
	minorver = atoi(minor);
	minorminorver = atoi(minorminor);

	if(majorver == 4 && minorver == 0 && minorminorver >= 2)
		return wyTrue;
	else if(IsMySQL41(tunnel, mysql))
		return wyTrue;
	else
		return wyFalse;
}	

wyBool
IsMySQL403(Tunnel * tunnel, PMYSQL mysql)
{
    wyString    ver;
	wyChar	    *major, *minor, *minorminor;
	wyChar	    seps[] = ".";
	wyInt32 	majorver, minorver, minorminorver;

	ver.SetAs(sja_mysql_get_server_info(tunnel, *mysql));

	major = strtok((wyChar*)ver.GetString(), seps);
	minor = strtok(NULL, seps);
	minorminor = strtok	(NULL, seps);

	majorver = atoi(major);
	minorver = atoi(minor);
	minorminorver = atoi(minorminor);

	if(majorver == 4 && minorver == 0 && minorminorver >= 3)
		return wyTrue;
	else if(IsMySQL41(tunnel, mysql))
		return wyTrue;
	else
		return wyFalse;
}	


wyBool
IsMySQL4013(Tunnel * tunnel, PMYSQL mysql)
{
	wyString    ver;
	wyChar	    *major, *minor, *minorminor;
	wyChar	    seps[] = ".";
	wyInt32		majorver, minorver, minorminorver;

	ver.SetAs(sja_mysql_get_server_info(tunnel, *mysql));

	major = strtok((wyChar*)ver.GetString(), seps);
	minor = strtok(NULL, seps);
	minorminor = strtok	(NULL, seps);

	majorver = atoi(major);
	minorver = atoi(minor);
	minorminorver = atoi(minorminor);

	if(majorver == 4 && minorver == 0 && minorminorver >= 13)
		return wyTrue;
	else if(IsMySQL41(tunnel, mysql))
		return wyTrue;
	else
		return wyFalse;
}	


wyBool
IsMySQL4018(Tunnel * tunnel, PMYSQL mysql)
{
	wyString ver;
	wyChar	*major, *minor, *minorminor;
	wyChar	seps[] = ".";
	wyInt32		majorver, minorver, minorminorver;

	ver.SetAs(sja_mysql_get_server_info(tunnel, *mysql));

	major = strtok((wyChar*)ver.GetString(), seps);
	minor = strtok(NULL, seps);
	minorminor = strtok	(NULL, seps);

	majorver = atoi(major);
	minorver = atoi(minor);
	minorminorver = atoi(minorminor);

	if(majorver == 4 && minorver == 0 && minorminorver >= 18)
		return wyTrue;
	else if(IsMySQL41(tunnel, mysql))
		return wyTrue;
	else
		return wyFalse;	
}	


wyBool
IsMySQL411(Tunnel * tunnel, PMYSQL mysql)
{
	long me = mysql_get_server_version(*mysql);/* Only available from MySQLv4.1*/

	if(me >= 40101)
		return wyTrue;
	else
		return wyFalse;
}

wyBool
IsMySQL412(Tunnel * tunnel, PMYSQL mysql)
{
	long me = mysql_get_server_version(*mysql);/* Only available from MySQLv4.12*/

	if(me >= 40102)
		return wyTrue;
	else
		return wyFalse;
}	

wyBool
IsMySQL5(Tunnel * tunnel, PMYSQL mysql)
{

	long me = mysql_get_server_version(*mysql);/* Only available from MySQLv4.1*/

	if(me >= 50000)
		return wyTrue;
	else
		return wyFalse;

}

wyBool
IsMySQL5010(Tunnel * tunnel, PMYSQL mysql)
{
	wyUInt32 me = mysql_get_server_version(*mysql);/* Only available from MySQLv4.1*/

	if(me >= 50010)
		return wyTrue;
	else
		return wyFalse;
}
	
wyBool
IsMySQL5017(Tunnel * tunnel, PMYSQL mysql)
{
	wyUInt32 me = mysql_get_server_version(*mysql);/* Only available from MySQLv4.1*/

	if(me >= 50017)
		return wyTrue;
	else
		return wyFalse;
}
	

wyBool 
IsMySQL516(Tunnel * tunnel, PMYSQL mysql)
{
	wyUInt32 me = mysql_get_server_version(*mysql);/* Only available from MySQLv4.1*/

	if(me >= 50106)
		return wyTrue;
	else
		return wyFalse;
}
	

wyBool
IsMySQL51(Tunnel * tunnel, PMYSQL mysql)
{
	long me = mysql_get_server_version(*mysql);/* Only available from MySQLv4.1*/

	if(me >= 50100)
		return wyTrue;
	else
		return wyFalse;

}

//MySQL 5.0.38
wyBool
IsMySQL5038(Tunnel * tunnel, PMYSQL mysql)
{
	long me = mysql_get_server_version(*mysql);/* Only available from MySQLv4.1*/

	if(me >= 50038)
		return wyTrue;
	else
		return wyFalse;
}

//MySQL 5.0.37
wyBool
IsMySQL5037(Tunnel * tunnel, PMYSQL mysql)
{
	long me = mysql_get_server_version(*mysql);/* Only available from MySQLv4.1*/

	if(me >= 50037)
		return wyTrue;
	else
		return wyFalse;
}

//MySQL 5.5.3
wyBool
IsMySQL553(Tunnel * tunnel, PMYSQL mysql)
{
	long me = mysql_get_server_version(*mysql);/* Only available from MySQLv4.1*/

	if(me >= 50503)
		return wyTrue;
	else
		return wyFalse;
}

//MySQL 5.5.10
wyBool
IsMySQL5510(Tunnel * tunnel, PMYSQL mysql)
{
	long me = mysql_get_server_version(*mysql);/* Only available from MySQLv4.1*/

	if(me >= 50510)
		return wyTrue;
	else
		return wyFalse;
}
wyBool
IsMySQL5500(Tunnel * tunnel, PMYSQL mysql)
{
	long me = mysql_get_server_version(*mysql);/* Only available from MySQLv4.1*/

	if(me >= 50500)
		return wyTrue;
	else
		return wyFalse;
}

wyBool
IsMySQL5600(Tunnel * tunnel, PMYSQL mysql)
{
	long me = mysql_get_server_version(*mysql);/* Only available from MySQLv4.1*/
	const char *dbString = mysql_get_server_info(*mysql);

	if(me >= 50600 && !strstr(dbString, "MariaDB"))
		return wyTrue;
	else
		return wyFalse;
}

wyBool
IsMySQL563(Tunnel *tunnel, PMYSQL mysql)
{
    long me = mysql_get_server_version(*mysql);/* Only available from MySQLv4.1*/
    const char *dbString = mysql_get_server_info(*mysql);

	if(me >= 50603 && !strstr(dbString, "MariaDB"))
		return wyTrue;
	else
		return wyFalse;
}

wyBool
IsMySQL565(Tunnel *tunnel, PMYSQL mysql)
{
    long me = mysql_get_server_version(*mysql);/* Only available from MySQLv4.1*/
    const char *dbString = mysql_get_server_info(*mysql);

	if(me >= 50605 && !strstr(dbString, "MariaDB"))
		return wyTrue;
	else
		return wyFalse;
}

//fractional seconds support
wyBool
IsMySQL564MariaDB53(Tunnel *tunnel, PMYSQL mysql)
{
    long me = mysql_get_server_version(*mysql);/* Only available from MySQLv4.1*/
    const char *dbString = mysql_get_server_info(*mysql);

	if((me >= 50604 && !strstr(dbString, "MariaDB"))||(me >= 50300 && strstr(dbString, "MariaDB")))
		return wyTrue;
	else
		return wyFalse;
}

wyBool
IsMySQL553MariaDB55(Tunnel *tunnel, PMYSQL mysql)
{
    long me = mysql_get_server_version(*mysql);/* Only available from MySQLv4.1*/
    const char *dbString = mysql_get_server_info(*mysql);

	if((me >= 50503 && !strstr(dbString, "MariaDB"))||(me >= 50500 && strstr(dbString, "MariaDB")))
		return wyTrue;
	else
		return wyFalse;
}


void GetVersionInfoforAutoComplete(MYSQL *mysql, wyString &VersionS)
{
	long me = mysql_get_server_version(mysql);
	char *dbString = mysql_get_server_info(mysql);
	if(strstr(dbString, "MariaDB")) ///if its mariadb,check if the version is above 10.2
	{
		if(me >= 100200) 
			me = 50713;///if mariadb version is > 10.2 it supports JSON, hence include functions till mysql version 5.7.13
		else
			me = 50066;///if mariadb version is < 10.2, include functions only for previous versons of mysql.		
	}
	
 	VersionS.Sprintf("%ld", me);
}

//DEFAULT, ON UPDATE clause for DATETIME 
wyBool
IsMySQL565MariaDB1001(Tunnel *tunnel, PMYSQL mysql)
{
    long me = mysql_get_server_version(*mysql);/* Only available from MySQLv4.1*/
    const char *dbString = mysql_get_server_info(*mysql);

	if((me >= 50605 && !strstr(dbString, "MariaDB")) || (me >= 100001 && strstr(dbString, "MariaDB")))
		return wyTrue;
	else
		return wyFalse;
}

wyBool IsMariaDB52(Tunnel * tunnel, PMYSQL mysql)
{
	long me = mysql_get_server_version(*mysql);
	const char *dbString = mysql_get_server_info(*mysql);
	
	if(me >= 50002 && strstr(dbString, "MariaDB"))
		return wyTrue;
	else
		return wyFalse;


}

wyBool IsMySQL57(Tunnel * tunnel, PMYSQL mysql)
{
	long me = mysql_get_server_version(*mysql);
	const char *dbString = mysql_get_server_info(*mysql);

	if(me >= 50700 && !strstr(dbString, "MariaDB") )
		return wyTrue;
	else
		return wyFalse;


}

wyBool IsMySQL573(Tunnel * tunnel, PMYSQL mysql)
{
	long me = mysql_get_server_version(*mysql);
	const char *dbString = mysql_get_server_info(*mysql);

	if (me >= 50703 && !strstr(dbString, "MariaDB"))
		return wyTrue;
	else
		return wyFalse;


}

// MySQL 5.7.8
wyBool IsMySQL578(Tunnel * tunnel, PMYSQL mysql)
{
	long me = mysql_get_server_version(*mysql);
	const char *dbString = mysql_get_server_info(*mysql);

	if(me >= 50708 && !strstr(dbString, "MariaDB") )
		return wyTrue;
	else
		return wyFalse;
}

// Mysql 5.7.6 and MariaDb 5.2 for virtual column support
wyBool IsMySQL576Maria52(Tunnel * tunnel, PMYSQL mysql)
{
	long me = mysql_get_server_version(*mysql);
	const char *dbString = mysql_get_server_info(*mysql);

	if((me >= 50706 && !strstr(dbString, "MariaDB") ) || (me >= 50200 && strstr(dbString, "MariaDB") ))
		return wyTrue;
	else
		return wyFalse;
}


/*
wyBool IsClusterDb(Tunnel * tunnel, PMYSQL mysql)
{
	long me = mysql_get_server_version(*mysql);

	const char *dbString = mysql_get_server_info(*mysql);

	if(me >= 50708 && strstr(dbString, "ClusterDB") )
		return wyTrue;
	else
		return wyFalse;
}
*/

//MySQL 5.7.7
wyBool
IsMySQL577(Tunnel * tunnel, PMYSQL mysql)
{
	long me = mysql_get_server_version(*mysql);/* Only available from MySQLv4.1*/

	if(me >= 50707)
		return wyTrue;
	else
		return wyFalse;
}

wyBool
IsMySQL502(Tunnel * tunnel, PMYSQL mysql)
{
	long me = mysql_get_server_version(*mysql);
	
	if(me >= 50002)
		return wyTrue;
	else
		return wyFalse;
}

wyBool
IsAlterOK(Tunnel * tunnel, PMYSQL mysql)
{
    wyString    ver;
	wyChar	    *major, *minor, *minorminor;
	wyChar	    seps[] = ".-";
	wyInt32	    majorver, minorver, minorminorver;

	ver.SetAs(sja_mysql_get_server_info(tunnel, *mysql));

	major = strtok((wyChar*)ver.GetString(), seps);
	minor = strtok(NULL, seps);
	minorminor = strtok(NULL, seps);

	majorver	= atoi(major);
	minorver	= atoi(minor);
	minorminorver	= atoi(minorminor);

	if((majorver == 3)&&(minorver == 23)&& minorminorver < 50)
		return wyFalse;
	
	return wyTrue;
}	


/*
  Quote a table name so it can be used in "SHOW TABLES LIKE <tabname>"

  SYNOPSIS
    quote_for_like()
    name     name of the table
    buff     quoted name of the table

  DESCRIPTION
    Quote \, _, ' and % characters

    Note: Because MySQL uses the C escape syntax in strings
   (for example, '\n' to represent newline), you must double
    any '\' that you use in your LIKE  strings. For example, to
    search for '\n', specify it as '\\n'. To search for '\', specify
    it as '\\\\'(the backslashes are stripped once by the parser
    and another time when the pattern match is done, leaving a
    single backslash to be matched).

    Example: "t\1" => "t\\\\1"

*/
 wyChar *quote_for_like(const wyChar *name, wyChar *buff)
{
  wyChar *to= buff;
  *to++= '\'';
  while(*name)
  {
    if(*name == '\\')
    {
      *to++='\\';
      *to++='\\';
      *to++='\\';
    }
    else if(*name == '\'' || *name == '_'  || *name == '%')
      *to++= '\\';
    *to++= *name++;
  }
  to[0]= '\'';
  to[1]= 0;
  return buff;
}

void 
SetCharacterSet(Tunnel *tunnel, MYSQL * mysql, wyChar * charset)
{

	wyChar			charval[][50] = {
										"Set character_set_connection=",
										"Set character_set_results=",
										"Set character_set_client="
	};
	wyChar			dbchar[] = "character_set_database";
	wyString        query;
	wyInt32			i;
	wyInt32			size = sizeof(charval)/ sizeof(charval[0]);
	MYSQL_RES		*res, *res2;
	MYSQL_ROW		row;
	
	if(sja_mysql_istunnel(tunnel))
	{
		sja_mysql_setcharset(tunnel, charset);
		return;
	}

    if(IsMySQL41(tunnel, &mysql) == wyFalse)
        return;

	if(stricmp(charset, "[default]")== 0 || strlen(charset)== 0)
	{
		query.Sprintf("show variables like '%character%'");

        res = SjaExecuteAndGetResult(tunnel, &mysql, query);

		if(!res && sja_mysql_affected_rows(tunnel, mysql)== -1)
			return ;

		while(row = sja_mysql_fetch_row(tunnel, res))
		{
			if(row[0] && !stricmp(row[0], dbchar))
			{
				for(i=0; i<size; i++)
				{
					if(row[1])
					{
						query.Sprintf("%s%s", charval[i], row[1]);

                        res2 = SjaExecuteAndGetResult(tunnel, &mysql, query);
						if(!res2 && sja_mysql_affected_rows(tunnel, mysql)== -1)
							goto cleanup;
					
						sja_mysql_free_result(tunnel, res2);
					}
				}
				break;

			} 
			else 
			{
				continue;
			}
		}

	cleanup:

		sja_mysql_free_result(tunnel, res);
	}
	else
	{
		query.Sprintf("set names '%s'", charset);

        res = SjaExecuteAndGetResult(tunnel, &mysql, query);
        
        if(res)
		    sja_mysql_free_result(tunnel, res);
	}

    return ;
}
