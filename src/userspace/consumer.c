#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    FILE *fp;

    if (argc != 2) {
        printf("Usage: ./consumer <car_id>\n");
        printf("Example: ./consumer 1\n");
        return 1;
    }

    int car_id = atoi(argv[1]);

    if (car_id < 1) {
        printf("Invalid car ID!\n");
        return 1;
    }

    /* Write to /proc/parking_input */
    fp = fopen("/proc/parking_input", "w");
    if (!fp) {
        printf("Error: Cannot open /proc/parking_input\n");
        printf("Make sure kernel module is loaded!\n");
        return 1;
    }

    fprintf(fp, "exit %d", car_id);
    fclose(fp);

    printf("Car#%d removal requested\n", car_id);
    printf("Run: cat /proc/parking to see update\n");

    return 0;
}
