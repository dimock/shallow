#pragma once

/*************************************************************
  minidump.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/
#if ((defined _MSC_VER) && (defined USE_MINIDUMP))
extern LONG miniDump(_EXCEPTION_POINTERS *pExceptionInfo);
extern LONG TopLevelFilter( _EXCEPTION_POINTERS *pExceptionInfo );
#endif // microsoft compiler only
