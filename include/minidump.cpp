
/*************************************************************
  minidump.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include <xcommon.h>

#if ((defined _MSC_VER) && (defined USE_MINIDUMP))

#include <windows.h>
#include <Dbghelp.h>
#include <time.h>


typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION, PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);


LONG miniDump(struct _EXCEPTION_POINTERS *pExceptionInfo)
{

  LONG retval = EXCEPTION_CONTINUE_SEARCH;

  HMODULE hDll = NULL;
  char szDbgHelpPath[_MAX_PATH];
  char szExeName[MAX_PATH];
  char szDumpPath[_MAX_PATH];
  szExeName[0] = 0;

  if (GetModuleFileName( NULL, szDbgHelpPath, _MAX_PATH ))
  {
    char *pSlash = strrchr( szDbgHelpPath, '\\' );
    if (pSlash)
    {
      strcpy(szExeName, pSlash+1);
      char * pDot = strrchr(szExeName, '.');
      if ( pDot )
        *pDot = 0;
      *(pSlash+1) = 0;
      strcpy(szDumpPath, szDbgHelpPath);
      strcpy( pSlash+1, "dbghelp.dll" );
      hDll = ::LoadLibrary( szDbgHelpPath );
    }
  }

  if (hDll==NULL)
  {
    hDll = ::LoadLibrary("dbghelp.dll");
  }

  LPCTSTR szResult = NULL;

  if ( !hDll )
    return retval;

  MINIDUMPWRITEDUMP pMiniDump = (MINIDUMPWRITEDUMP)::GetProcAddress( hDll, "MiniDumpWriteDump" );
  if ( !pMiniDump )
    return retval;


  strcat( szDumpPath, szExeName );

  time_t curtime;
  time(&curtime);
  tm * t = localtime(&curtime);
  char strcurtime[MAX_PATH];
  strftime(strcurtime, MAX_PATH, "_%d-%m-%Y_%H-%M-%S", t);
  strcat(szDumpPath, strcurtime);
  strcat(szDumpPath, ".dmp");

  HANDLE hFile = ::CreateFile( szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, 
    CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL, NULL );

  if ( hFile!=INVALID_HANDLE_VALUE )
  {
    _MINIDUMP_EXCEPTION_INFORMATION ExInfo;

    ExInfo.ThreadId = ::GetCurrentThreadId();
    ExInfo.ExceptionPointers = pExceptionInfo;
    ExInfo.ClientPointers = NULL;

    BOOL bOK = pMiniDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpWithFullMemory/*MiniDumpNormal*/, &ExInfo, NULL, NULL );

    if ( bOK )
      retval = EXCEPTION_EXECUTE_HANDLER;

    ::CloseHandle(hFile);
  }

  return retval;
}

LONG TopLevelFilter( _EXCEPTION_POINTERS *pExceptionInfo )
{
  LONG retval = EXCEPTION_CONTINUE_SEARCH;
  __try
  {
    retval = miniDump(pExceptionInfo);
  }
  __finally
  {
  }

  return retval;
}

#endif // microsoft compiler only
