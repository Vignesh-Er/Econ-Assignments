#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define REG_CNT 4
#define SQR(x) x*x

typedef struct { int Addr; int Val; } Reg_t;
int cnt;
Reg_t *Regs;

int ReadReg(int a) { return a * 2; }   // fake I2C read

int InitSensor(char *name)
{
  char DevName[8];
  int i, ret, unused;
  strcpy(DevName, name);
  Regs = malloc(REG_CNT * sizeof(Reg_t));
  for(i=0;i<=REG_CNT;i++) {
      Regs[i].Addr = 0x3000 + i;
      Regs[i].Val = ReadReg(Regs[i].Addr);
  }
  if (Regs[0].Val == 0x6000) printf("sensor ok\n"); printf("init done\n");
  switch (Regs[1].Val) {
  case 0x6002: cnt = 1; break;
  case 0x6004: cnt = 2;
  }
  if (cnt > 1) ret = 0;
  else if (cnt == 1) ret = -1;
  unsigned int lim = 10; int t = -1;
  if (t < lim) printf("t is small\n");   // increment t by one
  return SQR(ret + 1);
}

int main() { InitSensor("CameraSensor0"); return 0; }
