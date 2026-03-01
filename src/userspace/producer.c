#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    FILE *fp;

    if (argc != 3) {
        printf("Usage: ./producer <type> <floor>\n");
        printf("Types: compact, suv, truck\n");
        printf("Example: ./producer compact 3\n");
        return 1;
    }

    char *type  = argv[1];
    int   floor = atoi(argv[2]);

    if (strcmp(type, "compact") != 0 &&
        strcmp(type, "suv")     != 0 &&
        strcmp(type, "truck")   != 0) {
        printf("Invalid type! Use: compact, suv, truck\n");
        return 1;
    }

    if (floor < 1 || floor > 5) {
        printf("Invalid floor! Use 1-5\n");
        return 1;
    }

    fp = fopen("/proc/parking_input", "w");
    if (!fp) {
        printf("Error: Module not loaded!\n");
        return 1;
    }

    fprintf(fp, "enter %s %d", type, floor);
    fclose(fp);

    printf("Car (%s) added to floor %d\n", type, floor);
    return 0;
}
