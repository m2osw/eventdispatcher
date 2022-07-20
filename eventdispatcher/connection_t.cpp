
// The following compiles properly, I would have to reimplement all
// the handlers properly and verify them to make the move to the version
// which would not require us to re-write subclasses each time

#include "connection_t.h"
#include "timer.h"

class handler_a
{
public:
    void process_hup()
    {
    }
};
class handler_b
{
public:
    void timeout_event()
    {
    }
};
handler_a g_handler_a;
ed::connection_template<ed::timer, decltype(g_handler_a)> g_test_a(&g_handler_a);
handler_b g_handler_b;
ed::connection_template<ed::timer, decltype(g_handler_b)> g_test_b(&g_handler_b);
