#include <chimp.h>

int
main (int argc, char **argv)
{
    if (!chimp_core_startup ()) {
        return 1;
    }

    chimp_core_shutdown ();
    return 0;
}

