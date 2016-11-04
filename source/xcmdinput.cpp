#include <xcmdqueue.h>
#include <xparser.h>

namespace NShallow
{

void xCmdQueue::setUci(bool uci)
{
  uci_ = uci;
}

bool xCmdQueue::isUci() const
{
  return uci_;
}

void xCmdQueue::push(xCmd const& cmd)
{
  if(cmd)
    commands_.push(cmd);
}

xCmd xCmdQueue::peek()
{
  auto str = input_.peekInput();
  if(str.empty())
    return{};
  return parse(str, uci_);
}

xCmd xCmdQueue::next()
{
  if(!commands_.empty())
  {
    auto cmd = commands_.front();
    commands_.pop();
  }
  return parse(input_.getInput(), uci_);
}

} // NShallow
