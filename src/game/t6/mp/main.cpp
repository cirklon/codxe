#include "pch.h"
#include "components/scr_parser.h"
#include "main.h"

namespace t6
{
namespace mp
{

T6_MP_Plugin::T6_MP_Plugin()
{
    DbgPrint("T6 MP: Registering modules\n");
    RegisterModule(new Config());
	RegisterModule(new scr_parser());
	DbgPrint("Putting this after scr_parser called\n");
}

} // namespace mp
} // namespace t6
