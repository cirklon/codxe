#pragma once

namespace t6
{
namespace mp
{

/* 8 */
enum scriptInstance_t
{
  SCRIPTINSTANCE_SERVER = 0x0,
  SCRIPTINSTANCE_CLIENT = 0x1,
  SCRIPT_INSTANCE_MAX = 0x2,
};

/* 3472 */
struct ScriptParseTree
{
  const char *name;
  int len;
  char *buffer;
};
} // namespace mp
} // namespace t6
