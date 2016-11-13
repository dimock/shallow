/*************************************************************
xcommand.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <xcommand.h>
#include <algorithm>
#include <sstream>
#include <boost/algorithm/string.hpp>

namespace NShallow
{

xCmd::xCmd(xType type) :
  type_(type)
{}

xCmd::xCmd(xType type, std::string&& str) :
  type_(type), str_(std::move(str))
{}

xCmd::xCmd(xType type, std::vector<std::string>&& moves) :
  type_(type), moves_(std::move(moves))
{}

xCmd::xCmd(xType type, std::string&& fen, std::vector<std::string>&& moves) :
  type_(type), str_(std::move(fen)), moves_(std::move(moves))
{}

xCmd::xCmd(xType type, int value) :
  type_(type), value_(value)
{}

xCmd::xCmd(xType type,
  std::map<std::string, int>&& params) :
  type_(type),
  params_(std::move(params))
{}

xCmd::xCmd(xType type, bool inf) :
  type_(type),
  infinite_(inf)
{}

xType xCmd::type() const
{
  return type_;
}

xCmd::operator bool() const
{
  return type_ != xType::xNone;
}

int xCmd::value() const
{
  return value_;
}

int xCmd::param(std::string const& name) const
{
  auto iter = params_.find(name);
  if(iter != params_.end())
    return iter->second;
  return -1;
}

std::string xCmd::param(size_t i) const
{
  if(i < moves_.size())
    return moves_[i];
  return "";
}

bool xCmd::infinite() const
{
  return infinite_;
}

std::string const& xCmd::fen() const
{
  return str_;
}

std::string const& xCmd::str() const
{
  return str_;
}

std::vector<std::string> const& xCmd::moves() const
{
  return moves_;
}

} // NShallow
