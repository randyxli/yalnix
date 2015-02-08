#include <unistd.h>

int main(int argc, char *argv[])
{
    char *arglist[1];
    arglist[0] = NULL;
    TracePrintf(1, "infinite_exec: About to Exec\n");
    Exec("./test/infinite_exec", arglist);
}
