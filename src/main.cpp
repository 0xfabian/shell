#include <shell.h>

int main(int argc, char** argv)
{
    Shell sh(argv[0]);

    sh.init();

    if (argc > 1)
        sh.run_file(argc, argv);
    else
        sh.run();
}