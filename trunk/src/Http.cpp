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

#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <process.h>
#include <wchar.h>
#include "Http.h"
#include "Global.h"
#include "UrlEncode.h"
#include "Verify.h"

#define		IGNORE_CERT         SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID
//#define		USER_AGENT			"Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.1.4322)"
//changing user agent to IE11 running in windows 8.1 in SQLyog version 12.08
#define		USER_AGENT			   "Mozilla/5.0 (compatible, MSIE 11, Windows NT 6.3; Trident/7.0;  rv:11.0) like Gecko"

void httplog(char * buff)
{/*
//#ifdef _DEBUG
//	FILE	*fp = fopen ( "E:\\http_log.log", "a" );
//	fprintf(fp, "%s\n" , buff);
//	fclose ( fp );
//#endif
*/
}

CHttp::CHttp ()

{
	m_url = m_Protocol = m_HostName = m_UserName = m_Password = m_FileName = NULL;
	m_Port = 0;

	m_contenttype = NULL;

	m_proxyport = 0;
	m_isproxy = false;
	m_proxy = m_proxyuser = m_proxypwd =NULL;

	m_challenge = false;
	m_challengename = m_challengepwd = NULL;

	m_HttpOpenRequest = m_InternetConnect = m_InternetSession = NULL;
	
	m_timeout = GetTimeOut();
}

CHttp::~CHttp ()
{
	/* clean up resources */

#ifdef _DEBUG2
    wyString w;

    if(m_url)
    {
        w.SetAs(m_url);
        httplog(w.GetLength());
    }

    if(m_Protocol)
    {
        w.SetAs(m_Protocol);
        httplog(w.GetLength());
    }

    if(m_HostName)
    {
        w.SetAs(m_HostName);
        httplog(w.GetLength());
    }

    if(m_UserName)
    {
        w.SetAs(m_UserName);
        httplog(w.GetLength());
    }

    if(m_Password)
    {
        w.SetAs(m_Password);
        httplog(w.GetLength());
    }

    if(m_FileName)
    {
        w.SetAs(m_FileName);
        httplog(w.GetLength());
    }

    if(m_proxy)
    {
        w.SetAs(m_proxy);
        httplog(w.GetLength());
    }

    if(m_proxyuser)
    {
        w.SetAs(m_proxyuser);
        httplog(w.GetLength());
    }

    if(m_proxypwd)
    {
        w.SetAs(m_proxypwd);
        httplog(w.GetLength());
    }

    if(m_challengename)
    {
        w.SetAs(m_challengename);
        httplog(w.GetLength());
    }

    if(m_challengepwd)
    {
        w.SetAs(m_proxypwd);
        httplog(w.GetLength());
    }


#endif
	if (m_url ) delete[] m_url;
	if (m_Protocol ) delete[] m_Protocol;
	if (m_HostName ) delete[] m_HostName;
	if (m_UserName ) delete[] m_UserName;
	if (m_Password ) delete[] m_Password;
	if (m_FileName ) delete[] m_FileName;

	if(m_contenttype) delete[] m_contenttype;

	if (m_proxy ) delete[] m_proxy;
	if (m_proxyuser ) delete[] m_proxyuser;
	if (m_proxypwd ) delete[] m_proxypwd;

	if (m_challengename ) delete[] m_challengename;
	if (m_challengepwd ) delete[] m_challengepwd;

	
	if (m_HttpOpenRequest ) InternetCloseHandle(m_HttpOpenRequest );
	if (m_InternetConnect ) InternetCloseHandle(m_InternetConnect );
	if (m_InternetSession ) InternetCloseHandle(m_InternetSession );
}

DWORD
CHttp::GetTimeOut()
{
	int		deftimeout  = 15000;

	return	deftimeout;
}

bool
CHttp::SetUrl (wyWChar * url )
{
	unsigned int	len;
	if (!url ) return wyFalse;
	if (m_url ) return wyFalse;

	len = wcslen (url ) + 1;

	m_url = new wyWChar[len];
	wcscpy (m_url, url );

	return BreakUrl ();
}

/* proxy server informations */

bool
CHttp::SetProxyInfo (bool isproxy,
					  const wyWChar * proxy, const wyWChar * proxyuser, const wyWChar * proxypwd,
					  int proxyport )
{
	m_isproxy = proxy;

	if (!proxy ) return true;

	if (m_proxy ) delete[] m_proxy;
	if (m_proxyuser ) delete[] m_proxyuser;
	if (m_proxypwd ) delete[] m_proxypwd;

	/* since proxy server information is always required as host:port we join the information
	   and keep it in the class */
	m_proxy = new wyWChar[wcslen(proxy) + 128];		/* 128 is for port number..extra buffer */
	
	swprintf (m_proxy, L"%s:%d", proxy, proxyport);

	m_proxyuser = new wyWChar[wcslen(proxyuser)+  1];
	wcscpy (m_proxyuser, proxyuser );

	m_proxypwd = new wyWChar[wcslen(proxypwd) + 1];
	wcscpy (m_proxypwd, proxypwd );

	m_proxyport = proxyport;

	return true;
}

/* challenge/response authentication info */

bool				
CHttp::SetChallengeInfo (bool challenge, const wyWChar * username, const wyWChar * pwd )
{

	/* check for error */
	if (!username || !pwd )
		return false;

	m_challenge = challenge;

	m_challengename = new wyWChar[wcslen(username) + 1];
	wcscpy (m_challengename, username );

	m_challengepwd = new wyWChar[wcslen(pwd) + 1];
	wcscpy (m_challengepwd, pwd );

	return true;
}

bool				
CHttp::SetContentType(const wyWChar * contenttype)
{
	if (m_contenttype ) return wyFalse;
	
	m_contenttype  = new wyWChar[wcslen(contenttype) + 1];
	wcscpy (m_contenttype , contenttype);

	return true;
}

bool
CHttp::BreakUrl ()
{
    wyWChar				temp[1];
    wyWChar				*canonicalurl;
    DWORD				requiredlen = 1;
    URL_COMPONENTS		urlcmp;
    
    urlcmp.dwStructSize = sizeof(urlcmp);

	// when initialized with NULL, the members of the structure will contain the
    // pointer to a the first char within the string. NO DYNAMIC ALLOCATION IS DONE
    urlcmp.lpszScheme		= NULL;
    urlcmp.lpszHostName		= NULL;
    urlcmp.lpszUserName		= NULL;
    urlcmp.lpszPassword		= NULL;
    urlcmp.lpszUrlPath		= NULL;
    urlcmp.lpszExtraInfo	= NULL;
    // The following lines set which components will be displayed. 
    urlcmp.dwSchemeLength	= 1;
    urlcmp.dwHostNameLength = 1;
    urlcmp.dwUserNameLength = 1;
    urlcmp.dwPasswordLength = 1;
    urlcmp.dwUrlPathLength	= 1;
    urlcmp.dwExtraInfoLength= 1;


    // Canonicalize URLs - remove unsafe chars    
    // find out the space required
    InternetCanonicalizeUrl (m_url, temp, &requiredlen, 0);

    canonicalurl = new wyWChar[requiredlen + 1];
    
    InternetCanonicalizeUrl (m_url, canonicalurl, &requiredlen, 0);
    canonicalurl[requiredlen] = 0;

    // delete the prev url buffer and copy the new contents
    delete[] m_url;
    m_url = new wyWChar[requiredlen + 1];
    wcscpy (m_url, canonicalurl );
    delete[] canonicalurl;
    
    if (!InternetCrackUrl (m_url, wcslen(m_url), 0, &urlcmp ))
	{        
        return false;
    }
    else {

        if (urlcmp.lpszScheme != 0) {
            m_Protocol = new wyWChar[urlcmp.dwSchemeLength + 1];
            wcsncpy(m_Protocol, urlcmp.lpszScheme, urlcmp.dwSchemeLength);
            m_Protocol[urlcmp.dwSchemeLength] = 0;
        } else
            m_Protocol = NULL;
        
        if (urlcmp.lpszHostName != 0) {
            m_HostName = new wyWChar[urlcmp.dwHostNameLength + 1];
            wcsncpy(m_HostName, urlcmp.lpszHostName, urlcmp.dwHostNameLength);
            m_HostName[urlcmp.dwHostNameLength] = '\0';
        } else
            m_HostName = NULL;
        
        m_Port = urlcmp.nPort;
        
        if (urlcmp.lpszUserName != 0) {
            m_UserName = new wyWChar[urlcmp.dwUserNameLength + 1];
            wcsncpy(m_UserName, urlcmp.lpszUserName, urlcmp.dwUserNameLength);
            m_UserName[urlcmp.dwUserNameLength] = '\0';
        } else
            m_UserName = NULL;
        
        if (urlcmp.lpszPassword != 0) {
            m_Password = new wyWChar[urlcmp.dwPasswordLength + 1];
            wcsncpy(m_Password, urlcmp.lpszPassword, urlcmp.dwPasswordLength);
            m_Password[urlcmp.dwPasswordLength] = '\0';
        } else
            m_Password = NULL;

        if (urlcmp.lpszUrlPath != 0) {
            m_FileName = new wyWChar[urlcmp.dwUrlPathLength + 1];
            wcsncpy(m_FileName, urlcmp.lpszUrlPath, urlcmp.dwUrlPathLength);
            m_FileName[urlcmp.dwUrlPathLength] = NULL;
        } else
            m_FileName = NULL;

        // append querystring to urlpath
        if (m_FileName && urlcmp.lpszExtraInfo ) {
			wyWChar		*temp = m_FileName;
            m_FileName = new wyWChar[urlcmp.dwUrlPathLength  + urlcmp.dwExtraInfoLength + 1];
            wmemcpy (m_FileName, urlcmp.lpszUrlPath, urlcmp.dwUrlPathLength );
            wmemcpy (m_FileName + urlcmp.dwUrlPathLength, urlcmp.lpszExtraInfo, urlcmp.dwExtraInfoLength );
            m_FileName[urlcmp.dwUrlPathLength  + urlcmp.dwExtraInfoLength] = NULL;
			delete[] temp;
        }        
    }

	return true;
}

/* make all the connections and send the data */

bool
CHttp::SendData (char * data, unsigned long datalen, bool isbase64encoded, int * status, bool checkauth)
{

    DWORD           count = 0;

	if (!data )
		return false;

	/* Alloc HINTERNET handles */
	if ( !AllocHandles ( isbase64encoded, status, checkauth ) )
		return false;

retry:

	if (!PostData(data, datalen ) )
		return false;

	if (!CheckError (status ) )
    {

        if(count < 3 )
        {
            count++;
            goto retry;
        }

		return false;
    }

    count = 0;

    if (*status == HTTP_STATUS_DENIED || *status == HTTP_STATUS_PROXY_AUTH_REQ ) 
    {

        count++;

        if (count > 3 )
            return false;

        if (SetAuthDetails (m_HttpOpenRequest, *status ))
            goto retry;

    }

	return true;
}

/* allocate various handles */

bool
CHttp::AllocHandles ( bool isbase64, int *status, bool checkauth)
{

	DWORD		flags	=	INTERNET_FLAG_RELOAD | 
							INTERNET_FLAG_NO_CACHE_WRITE | 
							INTERNET_FLAG_KEEP_CONNECTION;
    unsigned long errnum;

	wyString contenttype,contenttypestr;
	//wyInt32     ret;
	
		

	/* 
		If a user has selected to use proxy server for Internet connection then we
		create a separate handle, send a dummy request and set the username, password 

		The real data connection and transfer is done in another handle */
	
	if (IsProxy () )
		m_InternetSession = InternetOpen (TEXT(USER_AGENT), INTERNET_OPEN_TYPE_PROXY, GetProxy(), NULL, 0 );
	else
		m_InternetSession = InternetOpen (TEXT(USER_AGENT), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0 );
	
	if (!m_InternetSession )
		return false;

	m_InternetConnect = InternetConnect (m_InternetSession, m_HostName, m_Port, m_UserName, m_Password, INTERNET_SERVICE_HTTP, 0L, 0L );
	if (!m_InternetConnect )
		return false;

	/* set the flags for internet connection  and check if SSL required */
	if (wcsicmp (m_Protocol, L"https" ) == 0 )
		flags |= INTERNET_FLAG_SECURE;

	/* check for proxy or password protected authentication 
	checkauth flag tells whether its required to be authenticated 
	*/
	if (checkauth && !Authorize (&errnum) )
    {
        *status = errnum;
		return false;
    }

	m_HttpOpenRequest = HttpOpenRequest(m_InternetConnect, L"POST", m_FileName, NULL, NULL, NULL, flags, 0L );
	if (!m_HttpOpenRequest )
		return false;

	//Content-Type 
	contenttype.SetAs(m_contenttype);
	contenttypestr.Sprintf("Content-Type: %s\r\n", contenttype.GetString());
	if (!HttpAddRequestHeaders(m_HttpOpenRequest, contenttypestr.GetAsWideChar () , (DWORD)-1, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE ) )
		return false;
				
	/*if (!HttpAddRequestHeaders(m_HttpOpenRequest, L"HTTP_USER_AGENT: Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.7.5) Gecko/20041107 Firefox/1.0\r\n", (DWORD)-1, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE ) )
		return false;*/
	//changing user string for avoid update your browser bug
	if (!HttpAddRequestHeaders(m_HttpOpenRequest, L"HTTP_USER_AGENT: Mozilla/5.0 (Windows; U;Windows NT 6.3; en-US; rv:36.0) Gecko/20100101 Firefox/36.0\r\n", (DWORD)-1, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE ) )
		return false;

	if (isbase64 ) {
		if (!HttpAddRequestHeaders(m_HttpOpenRequest, L"Base64: yes\r\n", (DWORD)-1, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE ) ) {
			assert (0 );
			return false;
		}
	}

	return true;
}

bool CHttp::SetAuthDetails (HINTERNET hRequest, DWORD httpstatus )
{

    const unsigned long     testsize = 2048;
	char					test[testsize]={0};
    DWORD                   ret;

    // We have to read all outstanding data on the Internet handle
	// before we can resubmit request. Just discard the data.
	do
	{
		InternetReadFile (hRequest, (LPVOID)test, testsize-1, &ret );
	}
	while (ret != 0 );

	DWORD			usrflg, pwdflg;
	const wyWChar	*usrname = {0}, *pwd = {0};

	if (httpstatus == HTTP_STATUS_DENIED ) {

		usrflg	= INTERNET_OPTION_USERNAME;
		pwdflg	= INTERNET_OPTION_PASSWORD;
		usrname = GetChallengeUserName();
		pwd		= GetChallengePwd();

	} else {

		usrflg	= INTERNET_OPTION_PROXY_USERNAME;
		pwdflg	= INTERNET_OPTION_PROXY_PASSWORD;
		usrname	= GetProxyUserName ();
		pwd		= GetProxyPwd ();

	}

	/* check that the information is valid */
	if (!usrname || !pwd )
		return false;

	if (!InternetSetOption (m_InternetConnect, usrflg, (LPVOID) usrname, wcslen (usrname ) ) )
        return false;

	if (!InternetSetOption (m_InternetConnect, pwdflg, (LPVOID) pwd, lstrlenW (pwd ) ) )
		return false;

    return true;
}



bool
CHttp::Authorize (unsigned long *num)
{
    INT						count=0;
    DWORD					numsize = sizeof(DWORD);
	DWORD					lasterr=0;
	DWORD					flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_KEEP_CONNECTION;
	HINTERNET				authrequest = NULL;

	/* set the flags for internet connection  and check if SSL required */
	if (wcsicmp (m_Protocol, L"https" ) == 0 )
		flags |= INTERNET_FLAG_SECURE;

	/* create a temporary internet request for proxy handling */
	authrequest = HttpOpenRequest(m_InternetConnect, L"POST", m_FileName, NULL, NULL, NULL, flags, 0L );
	if (!authrequest )
		return false;

retry:

    /* post a dummy request to check for auth */
	if (!yog_HttpSendRequest(&lasterr, authrequest, NULL, 0, NULL, 0) ) {

        if (lasterr == ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED ) {

            // Return ERROR_SUCCESS regardless of clicking on OK or Cancel
            if(lasterr = InternetErrorDlg(GetDesktopWindow(), 
                                    authrequest,
                                    ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED,
                                    FLAGS_ERROR_UI_FILTER_FOR_ERRORS       |
                                    FLAGS_ERROR_UI_FLAGS_GENERATE_DATA     |
                                    FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS, 
                                    NULL) != ERROR_SUCCESS )
            {
                goto cleanup;
            } else
			{
                goto retry;
			}
        }
		if ((lasterr == ERROR_INTERNET_INVALID_CA) ||
            (lasterr == ERROR_INTERNET_SEC_CERT_CN_INVALID) || 
            (lasterr == ERROR_INTERNET_SEC_CERT_DATE_INVALID )) {

                DWORD flags;
                DWORD flaglen = sizeof(flags);
			
			/*  this means the HTTP site is not authorised by a valid certification so we ignore it and
				move ahead */	
            InternetQueryOption (authrequest, INTERNET_OPTION_SECURITY_FLAGS, (LPVOID)&flags, &flaglen);
			flags |= IGNORE_CERT;
            if (!InternetSetOption (authrequest, INTERNET_OPTION_SECURITY_FLAGS, (LPVOID)&flags, sizeof (flags) ) )
				goto cleanup;
			else 
			{
				goto retry;
			}
        }
    }

    if ( !HttpQueryInfo ( authrequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,  (LPVOID)num, &numsize, NULL))
        goto cleanup;

	if ( *num == HTTP_STATUS_DENIED || *num == HTTP_STATUS_PROXY_AUTH_REQ )
	{
        count++;

        if (count > 3 )
            goto cleanup;

        if (SetAuthDetails ( authrequest, *num ))
		{
            goto retry;
		}
        else
            goto cleanup;
    }

    InternetCloseHandle (authrequest );
    // We don't want to descard the output

	return true;

cleanup:
	InternetCloseHandle (authrequest );
	return false;
}

/* thread and worker functions for httpsendrequest */
/* this technique is taken from MSDN KB 224318 */

BOOL
CHttp::yog_HttpSendRequest (	DWORD       *lpdwLastError,
                                HINTERNET	hRequest,
								LPCTSTR		lpszHeaders,
								DWORD		dwHeadersLength,
								LPVOID		lpOptional,
								DWORD		dwOptionalLength )
{
	HANDLE		thread;
	unsigned	threadid;
	DWORD		exitcode;

	HTTPSENDREQUESTPARAM	param={0};

	/* set the long param that wol;*/
	param.hRequest			= hRequest;
	param.lpszHeaders		= lpszHeaders;
	param.dwHeadersLength	= dwHeadersLength;
	param.lpOptional		= lpOptional;
	param.dwOptionalLength	= dwOptionalLength;
    param.dwLastError       = 0;

	thread = (HANDLE)_beginthreadex (	
							NULL,            // Pointer to thread security attributes
							0,               // Initial thread stack size, in bytes
							CHttp::worker_HttpSendRequest,// Pointer to thread function
							&param,     // The argument for the new thread
							0,               // Creation flags
							&threadid      // Pointer to returned thread identifier
						  );

	if (WaitForSingleObject (thread, m_timeout ) == WAIT_TIMEOUT )
	{
		InternetCloseHandle(m_InternetSession);
		m_InternetSession = NULL;
		WaitForSingleObject (thread, INFINITE);
		VERIFY (CloseHandle (thread ) );
		SetLastError (ERROR_INTERNET_TIMEOUT );
		return FALSE;
	}

    *lpdwLastError = param.dwLastError;
	exitcode = 0;
	if (!GetExitCodeThread(thread, &exitcode ) )
		return FALSE;

	VERIFY (CloseHandle (thread ) );

	return exitcode;
}

unsigned __stdcall 
CHttp::worker_HttpSendRequest(LPVOID  lp )
{
	
    DWORD           ret;
    
    HTTPSENDREQUESTPARAM		*param = (HTTPSENDREQUESTPARAM*)lp;

    ret = HttpSendRequest (	param->hRequest, param->lpszHeaders, param->dwHeadersLength,
								param->lpOptional, param->dwOptionalLength );

    param->dwLastError = GetLastError();
	return ret;

}

char*
CHttp::GetEncodedData (char * data, DWORD * encodeddatalen )
{
	*encodeddatalen = 0;

	DWORD		datalen = (strlen (data ) * 3) + 1;
	char		*encodereq = (char*) malloc (sizeof (char ) * datalen ); 

	CURLEncode	encode;

	if (!encode.URLEncode(data, encodereq, encodeddatalen ) )
		goto cleanup;

	return encodereq;

cleanup:

	free (encodereq );
	
	return NULL;
}

char*
CHttp::GetEncodedData(const char * data, char * outdata, DWORD * encodeddatalen )
{
	char		*encodereq		= outdata;
	DWORD		encodedlen = 0;
	CURLEncode	encode;

	if (!encode.URLEncode(data, encodereq, &encodedlen ) )
		goto cleanup;
	
	if (encodeddatalen ) *encodeddatalen = encodedlen;

	return outdata;

cleanup:

	return NULL;
}

bool
CHttp::PostData (char * encodeddata, DWORD encodelen )
{
    INTERNET_BUFFERS	bufferin;
	DWORD				flags = IGNORE_CERT, flaglen = 0, byteswritten=0, lasterr = 0;
   
	memset (&bufferin, 0, sizeof(INTERNET_BUFFERS));
    
    bufferin.dwStructSize	= sizeof(INTERNET_BUFFERS ); 
    bufferin.dwBufferTotal	= encodelen;

retry:

	if (!yog_HttpSendRequestEx (m_HttpOpenRequest, &bufferin, NULL, HSR_INITIATE, 0) )
	{
        lasterr = GetLastError();


        if (lasterr == ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED ) {

            // Return ERROR_SUCCESS regardless of clicking on OK or Cancel
            if(InternetErrorDlg(GetDesktopWindow(), 
                                    m_HttpOpenRequest,
                                    ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED,
                                    FLAGS_ERROR_UI_FILTER_FOR_ERRORS       |
                                    FLAGS_ERROR_UI_FLAGS_GENERATE_DATA     |
                                    FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS, 
                                    NULL) != ERROR_SUCCESS )
                return false;
            else
                goto retry;

        } else if (lasterr == ERROR_INTERNET_CANNOT_CONNECT ) {
			return false;

		} else if ((lasterr == ERROR_INTERNET_INVALID_CA ) || 
					(lasterr ==  ERROR_INTERNET_SEC_CERT_CN_INVALID ) ||
					(lasterr == ERROR_INTERNET_SEC_CERT_DATE_INVALID )	
				) {
			
				InternetQueryOption (m_HttpOpenRequest, INTERNET_OPTION_SECURITY_FLAGS,
					 (LPVOID)&flags, &flaglen);

            flags |= IGNORE_CERT;
            InternetSetOption (m_HttpOpenRequest, INTERNET_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));
            
			goto retry;

        } else {
			return false;
        }
    }	

	if (!yog_InternetWriteFile(m_HttpOpenRequest, encodeddata, encodelen, &byteswritten ) )
		return false;

	return true;
}

/* thread and worker functions for httpsendrequestex */
/* this technique is taken from MSDN KB 224318 */

BOOL
CHttp::yog_HttpSendRequestEx(	HINTERNET hRequest, LPINTERNET_BUFFERS lpBuffersIn, 
								LPINTERNET_BUFFERS lpBuffersOut, 
								DWORD dwFlags, DWORD_PTR dwContext )
{
	HANDLE		thread;
	unsigned	threadid;
	DWORD		exitcode;

	HTTPSENDREQUESTEXPARAM	param={0};

	/* set the long param that wol;*/
	param.hRequest = hRequest;
	param.lpBufferIn = lpBuffersIn;
	param.lpBuffersOut = lpBuffersOut;
	param.dwFlags = dwFlags;
	param.dwContext = dwContext;

	thread = (HANDLE)_beginthreadex (	
							NULL,            // Pointer to thread security attributes
							0,               // Initial thread stack size, in bytes
							CHttp::worker_HttpSendRequestEx,  // Pointer to thread function
							&param,     // The argument for the new thread
							0,               // Creation flags
							&threadid      // Pointer to returned thread identifier
						  );

	if (WaitForSingleObject (thread, m_timeout ) == WAIT_TIMEOUT )
	{
		InternetCloseHandle(m_InternetSession);
		m_InternetSession = NULL;
		WaitForSingleObject (thread, INFINITE);
		VERIFY (CloseHandle (thread ) );
		SetLastError (ERROR_INTERNET_TIMEOUT );
		return FALSE;
	}

	exitcode = 0;
	if (!GetExitCodeThread(thread, &exitcode ) )
		return FALSE;

	VERIFY (CloseHandle (thread ) );

	return exitcode;
}

unsigned __stdcall 
CHttp::worker_HttpSendRequestEx(LPVOID  lp )
{
	HTTPSENDREQUESTEXPARAM		*param = (HTTPSENDREQUESTEXPARAM*)lp;

	return HttpSendRequestEx (	param->hRequest, param->lpBufferIn, param->lpBuffersOut, 
								param->dwFlags, param->dwContext );
}

/* thread and worker functions for internetwritefile */
/* this technique is taken from MSDN KB 224318 */

BOOL
CHttp::yog_InternetWriteFile (HINTERNET hFile, LPCVOID lpBuffer, 
							  DWORD dwNumberOfBytesToWrite, LPDWORD lpdwNumberOfBytesWritten )
{
	HANDLE				thread;
	unsigned 			threadid;
	DWORD				exitcode;

	INTERNETWRITEFILE	param={0};

	/* set the long param that wol;*/
	param.dwNumberOfBytesToWrite = dwNumberOfBytesToWrite;
	param.hFile = hFile;
	param.lpBuffer = lpBuffer;
	param.lpdwNumberOfBytesWritten = lpdwNumberOfBytesWritten;

	thread = (HANDLE)_beginthreadex (	
							NULL,            // Pointer to thread security attributes
							0,               // Initial thread stack size, in bytes
							CHttp::worker_InternetWriteFile,  // Pointer to thread function
							&param,     // The argument for the new thread
							0,               // Creation flags
							&threadid      // Pointer to returned thread identifier
						  );

	if (WaitForSingleObject (thread, m_timeout ) == WAIT_TIMEOUT )
	{
		InternetCloseHandle(m_InternetSession);
		m_InternetSession = NULL;
		WaitForSingleObject (thread, INFINITE);
		VERIFY (CloseHandle (thread ) );
		SetLastError (ERROR_INTERNET_TIMEOUT );
		return FALSE;
	}

	exitcode = 0;
	if (!GetExitCodeThread(thread, &exitcode ) )
		return FALSE;

	VERIFY (CloseHandle (thread ) );

	return exitcode;
}

unsigned __stdcall 
CHttp::worker_InternetWriteFile(LPVOID  lp )
{
	INTERNETWRITEFILE		*param = (INTERNETWRITEFILE*)lp;

	return InternetWriteFile (	param->hFile, param->lpBuffer, 
								param->dwNumberOfBytesToWrite, param->lpdwNumberOfBytesWritten );
}


bool
CHttp::CheckError (int * status )
{
	DWORD num, numsize;
	if (!yog_HttpEndRequest (m_HttpOpenRequest, NULL, HSR_INITIATE, 0)) {

        DWORD           qcode = 0;
        DWORD           qsize = sizeof (DWORD );

        // checks whether the server requires proxy auth?
		::HttpQueryInfo (m_HttpOpenRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &qcode, &qsize, NULL);

        if (qcode == HTTP_STATUS_DENIED || qcode == HTTP_STATUS_PROXY_AUTH_REQ ) {
			/* we need to show status error */
			*status = qcode;
			return true;
        }

		/* for other return false as they are fatal */
		return false;
    }    

	/* get the status code */
	numsize = sizeof(num);
	
	if (!::HttpQueryInfo (m_HttpOpenRequest, HTTP_QUERY_STATUS_CODE | 
                                             HTTP_QUERY_FLAG_NUMBER,
                (LPVOID)&num, &numsize, NULL))
	{
		return false;
	}

	*status = num;

	return true;
}

/* thread and worker functions for httpendrequest */
/* this technique is taken from MSDN KB 224318 */

BOOL
CHttp::yog_HttpEndRequest (	HINTERNET hRequest,  LPINTERNET_BUFFERS lpBuffersOut,
							DWORD dwFlags,  DWORD dwContext )
{
	HANDLE				thread;
	unsigned			threadid;
	DWORD				exitcode;

	HTTPENDREQUEST		param={0};

	/* set the long param that wol;*/
	param.hRequest		= hRequest;
	param.lpBuffersOut	= lpBuffersOut;
	param.dwFlags		= dwFlags;
	param.dwContext		= dwContext;

	thread = (HANDLE)_beginthreadex (	
							NULL,            // Pointer to thread security attributes
							0,               // Initial thread stack size, in bytes
							CHttp::worker_HttpEndRequest,  // Pointer to thread function
							&param,     // The argument for the new thread
							0,               // Creation flags
							&threadid      // Pointer to returned thread identifier
						  );

	if (WaitForSingleObject (thread, m_timeout ) == WAIT_TIMEOUT )
	{
		InternetCloseHandle(m_InternetSession);
		m_InternetSession = NULL;
		WaitForSingleObject (thread, INFINITE);
		VERIFY (CloseHandle (thread ) );
		SetLastError (ERROR_INTERNET_TIMEOUT );
		return FALSE;
	}

	exitcode = 0;
	if (!GetExitCodeThread(thread, &exitcode ) )
		return FALSE;

	VERIFY (CloseHandle (thread ) );

	return exitcode;
}

unsigned __stdcall 
CHttp::worker_HttpEndRequest(LPVOID  lp )
{
	HTTPENDREQUEST		*param = (HTTPENDREQUEST*)lp;

	return HttpEndRequest (param->hRequest, param->lpBuffersOut, param->dwFlags, param->dwContext );
}


bool
CHttp::ReadResponse(char ** pbuffer, DWORD * pbuffersize, bool * stop )
{
    char*	buffer = 0; 	    // Partial chunks returned by InternerReadFile
	char*	finalbuffer = 0;	// The entire stream
    char*	tempbuffer = 0;		// For lpFinalBuffer reallocation

	// store the sizes of the above buffers
	DWORD	buffersize = 0, finalbuffersize = 0, tempbuffersize = 0;
	DWORD	downloaded = 0;	// size of data downloaded by InternetReadFile
	
	*pbuffer = 0;
    *pbuffersize = 0;
    
    // Loop to Read data
	do
	{		
		// Get the no. of bytes of data that are available to be read immediately
		// by a subsequent call to InternetReadFile			

		if (!InternetQueryDataAvailable (m_HttpOpenRequest, &buffersize, 0, 0 )) {

            if (finalbuffer )
				free(finalbuffer);
                //delete[] finalbuffer;
			return false;

        } else {

				if (buffersize == 0 )
					break;				

				// Allocate a buffer of the size returned by InternetQueryDataAvailable
				//buffer = new char[buffersize];
				buffer = (char*)calloc(buffersize, sizeof(char));
				
				/* check if the user has asked to stop the result bringing */
				if (stop && *stop ) {
					//delete[] buffer;
					free(buffer);

				if (finalbuffer )
					//delete[] finalbuffer;
					free(finalbuffer);

				return NULL;
			}

			// Read the data from the HttOpenRequest Handle
            if (!yog_InternetReadFile(m_HttpOpenRequest, buffer, buffersize, &downloaded)) {

                //delete[] buffer;
				free(buffer);

                if (finalbuffer )
                    //delete[] finalbuffer;
					free(finalbuffer);

                *pbuffer = 0;

				return false;
                    
            } else 	{

                // Downloaded with return 0 if the entire stream has been read				
				if (downloaded == 0) break;
				
				// For finalbuffer reallocation, store the final buffer address and size
				tempbuffer = finalbuffer;
				tempbuffersize = finalbuffersize;

				// Calculate the new size and reallocate final buffer 
				finalbuffersize = buffersize + tempbuffersize;
				
                //finalbuffer = new char[finalbuffersize+1];
				finalbuffer = (char*)calloc(finalbuffersize+1, sizeof(char));
					
				// Copy the contents of the temp buffer into final buffer
				if (tempbuffer ) memcpy (finalbuffer, tempbuffer, tempbuffersize );

				// Finally copy the current buffer received into final buffer
				memcpy (finalbuffer + tempbuffersize, buffer, buffersize );

				// clear
                if (tempbuffer ) {
                    //delete[] tempbuffer;
					free(tempbuffer);
                    tempbuffer = NULL;
                }
				
                if (buffer ) {
                    //delete[] buffer;
					free(buffer);
                    buffer = NULL;
                }
			}
		}
	}
	while(true);

	/* sometimes PHP might echo 0 zero bytes */
	if (!finalbuffersize )	{

		//*pbuffer = new char[1];
		*pbuffer = (char*)calloc(1, sizeof(char));
		*pbuffer[0] = 0;
		*pbuffersize = 0;
		return true;
	}
    
	finalbuffer[finalbuffersize] = NULL;
	*pbuffer = finalbuffer;
	*pbuffersize = finalbuffersize;

	return true;	
}

/* thread and worker functions for internetreadfile */
/* this technique is taken from MSDN KB 224318 */

BOOL
CHttp::yog_InternetReadFile (HINTERNET hFile, LPVOID lpBuffer, 
							  DWORD dwNumberOfBytesToRead, LPDWORD lpdwNumberOfBytesRead )
{
	HANDLE				thread;
	unsigned			threadid;
	DWORD				exitcode;

	INTERNETREADFILE	param={0};

	/* set the long param ;*/
	param.dwNumberOfBytesToRead		= dwNumberOfBytesToRead;
	param.hFile						= hFile;
	param.lpBuffer					= lpBuffer;
	param.lpdwNumberOfBytesRead		= lpdwNumberOfBytesRead;

	thread = (HANDLE)_beginthreadex (	
							NULL,            // Pointer to thread security attributes
							0,               // Initial thread stack size, in bytes
							CHttp::worker_InternetReadFile,  // Pointer to thread function
							&param,     // The argument for the new thread
							0,               // Creation flags
							&threadid      // Pointer to returned thread identifier
						  );

	if (WaitForSingleObject (thread, m_timeout ) == WAIT_TIMEOUT )
	{
		InternetCloseHandle(m_InternetSession);
		m_InternetSession = NULL;
		WaitForSingleObject (thread, INFINITE);
		VERIFY (CloseHandle (thread ) );
		SetLastError (ERROR_INTERNET_TIMEOUT );
		return FALSE;
	}

	exitcode = 0;
	if (!GetExitCodeThread(thread, &exitcode ) )
		return FALSE;

	VERIFY (CloseHandle (thread ) );

	return exitcode;
}

unsigned __stdcall 
CHttp::worker_InternetReadFile(LPVOID  lp )
{
	INTERNETREADFILE		*param = (INTERNETREADFILE*)lp;

	return InternetReadFile (	param->hFile, param->lpBuffer, 
								param->dwNumberOfBytesToRead, param->lpdwNumberOfBytesRead );
}

char*
CHttp::GetResponse(bool * stop )
{
	char		*response=NULL;
	DWORD		responselen = 0;

	if (!ReadResponse (&response, &responselen, stop  ) )
		return NULL;

	return response;
}

/* function returns all the headers returned by a post.
   receives all the headers returned by the server. Each header is separated by a carriage return/line feed (CR/LF) sequence. 
   it does only one call so it assumes that calling function passes big enuf buffer at once 

   return TRUE or FALSE */

bool
CHttp::GetAllHeaders (wyWChar * buffer, DWORD bufsize )
{
	return (bool)::HttpQueryInfo (m_HttpOpenRequest, HTTP_QUERY_RAW_HEADERS_CRLF, (LPVOID)buffer, 
							 &bufsize, NULL );
}