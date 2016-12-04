/*************************************************************
xtests.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#pragma once

#include <Helpers.h>
#include <list>
#include <functional>

namespace NEngine
{

namespace xtests_details_ns
{
using FBoard = SBoard<16>;
};

class FenTest : public std::list<std::pair<xtests_details_ns::FBoard, Move>>
{

public:
  FenTest(std::string const& ffname);
};

using xTestFen_Callback = std::function<void(Board&, Move&)>;

void testFen(std::string const& ffname, xTestFen_Callback const&);


} // NEngine
