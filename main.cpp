#include "disassembler.cpp"
#include "runtime.cpp"

void print_help()
{
    fprintf(stderr, "Usage:\n\tchipperino -d <file>\n\tchipperino -e <file>\n");
}

int main(int argc, char *argv[])
{
    char *filename = NULL;
    enum { NONE, DISASSEMBLE, EXECUTE };
    int action = NONE;
    
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i][0] != '-')
        {
            // if not preceded by '-', assume arg is the target filename
            filename = argv[i];
        }
        if (!strcmp("-d", argv[i]))
        {
            action = DISASSEMBLE;
        }
        if (!strcmp("-e", argv[i]))
        {
            action = EXECUTE;
        }
    }

    switch (action)
    {
    case DISASSEMBLE:
        disassemble(filename);
        break;

    case EXECUTE:
        execute(filename);
        break;
        
    case NONE:
        print_help();
        return 1;
        break;
    }
}
