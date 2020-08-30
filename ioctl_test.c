#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>



static char scancode[128] = "\0\e1234567890-=\177\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\000789-456+1230.\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
static int shift = 0;


 /*
 * So this test ioctl number means that the user space is sending a message to the kernel
 * */
#define IOCTL_TEST _IOW(0, 6, struct ioctl_test_t)

int main () {

  /* attribute structures */
  struct ioctl_test_t {
    int field1;
    char field2;
  } ioctl_test;


  // this is the file descriptor for the psuedo device file /proc/ioctl_test
  // the pseudo device file is created during the initialization routine
  int fd = open ("/proc/ioctl_test", O_RDONLY);

  ioctl_test.field1 = 10;
  ioctl_test.field2 = 'a';

  int retVal = ioctl (fd, IOCTL_TEST, &ioctl_test);
  if(retVal < 0) {
    printf("Sending message to ioctl failed");
  }

  unsigned short buf;
  int sizeOfBuf;
  int i = 0;

  printf("\n");
  while (1)
  {
    sizeOfBuf = read(fd, &buf, 2);
    i++;
    
    unsigned short val = (buf & 0x00FF);

    if (val == 0x01) {exit(0);}
    if (val == 0x2A) {shift = 1;}
    if (val == 0xAA) {shift = 0;}   
    if (val < 128)
    { 
      char out = scancode[val];
      int c_val = out;

      if (shift == 1 && (c_val < 123 && c_val > 60)) {
        printf("%c", (c_val -= 32));
      }
      else{
          printf("%c", c_val);
      }
    }

  }
  close(fd);
 
  return 0;
}