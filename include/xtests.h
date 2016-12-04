/*************************************************************
xtests.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#pragma once

#include <Helpers.h>
#include <list>
#include <functional>

namespace NEngine
{

using xTestFen_Callback = std::function<void(size_t, Board&, Move&)>;
using xTestFen_ErrorCallback = std::function<void(std::string const&)>;

namespace xtests_details_ns
{
using FBoard = SBoard<16>;
};

class FenTest : public std::vector<std::pair<xtests_details_ns::FBoard, Move>>
{

public:
  FenTest(std::string const& ffname, xTestFen_ErrorCallback const&);
};

void testFen(std::string const& ffname, xTestFen_Callback const&, xTestFen_ErrorCallback const&);
void testSee(std::string const& ffname);

} // NEngine
