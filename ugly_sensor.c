#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REG_CNT              4
#define REG_BASE_ADDR        0x3000
#define SENSOR_CHIP_ID_EXP   0x6000
#define SENSOR_MODE_NORMAL   0x6002
#define SENSOR_MODE_BURST    0x6004

#define SQR(x)               ((x) * (x))

typedef struct {
    int address;
    int value;
} Register;

/**
 * @brief Test mock simulating an I2C register read. Must be replaced with real bus I/O in production.
 */
int ReadReg(int addr, int *val)
{
    if (val == NULL) {
        return -1;
    }
    *val = addr * 2;
    return 0;
}

int InitSensor(const char *name)
{
    char DevName[32];
    int i, ret = -1;
    int cnt = 0;
    int status = -1;
    Register *Regs = NULL;
    int lim = 10;
    int t = -1;

    if (name == NULL) {
        return -1;
    }

    strncpy(DevName, name, sizeof(DevName) - 1);
    DevName[sizeof(DevName) - 1] = '\0';
    (void)DevName;

    Regs = malloc(REG_CNT * sizeof(Register));
    if (Regs == NULL) {
        return -1;
    }

    for (i = 0; i < REG_CNT; i++) {
        Regs[i].address = REG_BASE_ADDR + i;
        if (ReadReg(Regs[i].address, &Regs[i].value) != 0) {
            goto cleanup;
        }
    }

    if (Regs[0].value != SENSOR_CHIP_ID_EXP) {
        goto cleanup;
    }

    printf("sensor ok\n");
    printf("init done\n");

    switch (Regs[1].value) {
    case SENSOR_MODE_NORMAL:
        cnt = 1;
        break;
    case SENSOR_MODE_BURST:
        cnt = 2;
        break;
    default:
        cnt = 0;
        break;
    }

    if (cnt > 1) {
        ret = 0;
    } else if (cnt == 1) {
        ret = -1;
    } else {
        ret = -1;
    }

    if (t < lim) {
        printf("t is small\n");
        t++;
    }

    status = SQR(ret + 1);

cleanup:
    free(Regs);
    Regs = NULL;
    return status;
}

int main(void)
{
    int status = InitSensor("CameraSensor0");
    if (status != 0) {
        return 1;
    }
    return 0;
}