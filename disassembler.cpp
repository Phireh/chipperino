#include "disassembler.hpp"

char *filename;

int main(int argc, char *argv[])
{
    // Arg parsing
    if (argc == 1)
    {
        fprintf(stderr, "Usage: chipperino <file>\n");
        return 1;
    }
    else
    {
        filename = argv[1];
    }

    fill_instruction_info();
    
    FILE *file_handle = fopen(filename, "rb");
    chip8_instruction_t *p = &chip8.memory.as_words[program_offset/sizeof(chip8_instruction_t)];
    chip8_instruction_t i;

    // Read instructions one-by-one
    while (p < ((chip8_instruction_t *) &chip8.memory + 4096) && fread(&i, sizeof(chip8_instruction_t), 1, file_handle))
    {
        //printf("Loaded instruction 0x%02X%02X in offset %d\n", i.msb, i.lsb, memory_offset(p));
        *p = i;            // Save written instruction to memory 
        ++p;               // advance pointer
        program_size += 2; // and count bytes read
    }
    fclose(file_handle);

    printf("Disassembly:\n================\n");
    printf("%s\t%s\t%s\t%18s\n", "ADDR", "INST", "PARAMS", "MNEMONIC");

    // Read instructions from first available address to last written
    for (int j = program_offset/sizeof(chip8_instruction_t); j < (program_size + program_offset)/2; ++j)
    {
        chip8_instruction_t *i = &chip8.memory.as_words[j];
        instruction_info_t info = disassemble(*i);
        char param_info_string[18] = "";

        if (info.nparams == 1)
            sprintf(param_info_string, "%X ", info.params[0]);
        if (info.nparams == 2)
            sprintf(param_info_string, "%X, %X", info.params[0], info.params[1]);
        if (info.nparams == 3)
            sprintf(param_info_string, "%X, %X, %X", info.params[0], info.params[1], info.params[2]);
        
        printf("%x\t%02X%02X\t%-18s%s\n", memory_offset(i), i->msb, i->lsb, param_info_string, info.mnemonic.c_str());
    }
    printf("================\nend of disassembly\n");
}
