/*************************************************************
xInput.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#include <xinput.h>

#ifdef __GNUC__

#include <stdio.h>
#include <sys/ioctl.h> // For FIONREAD
#include <termios.h>
#include <stdbool.h>

#endif

namespace NShallow
{

xInput::xInput() :
  is_(std::cin)
{
#ifdef _MSC_VER
  hinput_ = GetStdHandle(STD_INPUT_HANDLE);
  if(hinput_)
  {
    DWORD mode = 0;
    in_pipe_ = !GetConsoleMode(hinput_, &mode);
    if(!in_pipe_)
    {
      BOOL ok = SetConsoleMode(hinput_, mode & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
      FlushConsoleInputBuffer(hinput_);
    }
  }
#elif (defined __GNUC__)
  const int STDIN = 0;
  struct termios term;
  tcgetattr(STDIN, &term);
  term.c_lflag &= ~ICANON;
  tcsetattr(STDIN, TCSANOW, &term);
  setbuf(stdin, NULL);
#endif
}

std::string xInput::peekInput()
{
  if(!peek())
    return std::string{};

  return readInput();
}

std::string xInput::getInput()
{
  return readInput();
}


std::string xInput::readInput()
{
  std::string sline;
  std::getline(is_, sline);
  return std::move(sline);
}

#ifdef _MSC_VER
bool xInput::peek()
{
  if(!hinput_)
    return false;

  if(in_pipe_)
  {
    DWORD avaliable = 0;
    if(!PeekNamedPipe(hinput_, 0, 0, 0, &avaliable, NULL))
      return false;

    return avaliable != 0;
  }
  else
  {
    DWORD num = 0;
    if(GetNumberOfConsoleInputEvents(hinput_, &num))
    {
      if(num == 0)
        return false;

      INPUT_RECORD irecords[256];
      DWORD nread = 0;
      if(PeekConsoleInput(hinput_, irecords, num, &nread))
      {
        for(DWORD i = 0; i < nread; ++i)
        {
          if(irecords[i].EventType & KEY_EVENT)
            return true;
        }
      }
    }
    return false;
  }
}
#elif (defined __GNUC__)
bool xInput::peek()
{
  const int STDIN = 0;
  int nbbytes;
  ioctl(STDIN, FIONREAD, &nbbytes);
  return nbbytes > 0;
}
#endif

}