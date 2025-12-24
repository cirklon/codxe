#pragma once

#include "pch.h"

namespace iw3
{
namespace mp
{
class gsc_functions : public Module
{
  public:
    gsc_functions();
    ~gsc_functions();

    const char *get_name() override
    {
        return "gsc_functions";
    };
};
} // namespace mp
} // namespace iw3
