#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fctl.h>
#include <sys/mman.h>

#define KEY 18
#define LED1 23
#define LED2 24
#define LED3 25
#define LOW 0
#define HIGH 1

#define IO_BASE 0x3F000000
#define GPIO_BASE (IO_BASE + 0x2000000)
#define GPIO_SIZE 256

#define GPFSEL0_OFFSET 0x0
#define GPSET0_OFFSET 0x1C
#define GPCLR0_OFFSET 0x28
#define GPLEV0_OFFSET 0x34

void set_gpio_input(int gpio_no, char* mmap_addr);
void set_gpio_output(int gpio_no, char* mmap_addr);
int gpio_lev(int gpio_no, char* mmap_addr);
void set_gpio_set(int gpio_no, char* mmap_addr);
void set_gpio_clear(int gpio_no, char* mmap_addr);
void controlled(int val, char* mmap_addr);
int clock = 0;
int press = 0;

int main(void)
{
	int fd;
	char* addr;

	fd = open("/dev/mem", O_RDWR | O_SYNC);

	if (fd < 0)
	{
		printf("open() error\n");
		return -1;
	}

	addr = mmap(NULL, GPIO_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO_BASE);

	if (addr == MAP_FAILED)
	{
		printf("mmap() error\n");
		return -1;
	}

	set_gpio_input(KEY, addr);
	set_gpio_output(LED1, addr);
	set_gpio_output(LED2, addr);
	set_gpio_output(LED3, addr);

	while (1)
	{
		int val = gpio_lev(KEY, addr);
		
		if (clock == 0)
		{
			controlled(val, addr);
		}
		clock = gpio_lev(KEY, addr);
		usleep(100000);
	}

	munmap(addr, GPIO_SIZE);
	close(fd);
	return 0;
}

void set_gpio_input(int gpio_no, char* mmap_addr)
{
	unsigned int* reg;
	int shift_amount;

	reg = (unsigned int*)(mmap_addr + GPFSEL0_OFFSET + (gpio_no / 10 * 4));
	shift_amount = (gpio_no % 10) * 3;
	*reg = *reg & ~(0b111 << shift_amount);
}

void set_gpio_output(int gpio_no, char* mmap_addr)
{
	unsigned int* reg;
	int shift_amount;

	reg = (unsigned int*)(mmap_addr + GPFSEL0_OFFSET + (gpio_no / 10 * 4));
	shift_amount = (gpio_no % 10) * 3;
	*reg = *reg | (1 << shift_amount);
}

int gpio_lev(int gpio_no, char* mmap_addr)
{
	unsigned int* reg;

	reg = (unsigned int*)(mmap_addr + GPLEV0_OFFSET);
	return *reg >> gpio_no & 1;
}

void gpio_set(int gpio_no, char* mmap_addr)
{
	unsigned int* reg;

	reg = (unsigned int*)(mmap_addr + GPSET0_OFFSET);
	return 1 << gpio_no;
}

void gpio_clear(int gpio_no, char* mmap_addr)
{
	unsigned int* reg;

	reg = (unsigned int*)(mmap_addr + GPCLR0_OFFSET);
	return 1 << gpio_no;
}

void controlled(int val, char* mmap_addr)
{
	if (val == HIGH)
	{
		press += 1;
		press %= 5;

		printf("Key is pressed\r\n");

		switch (press)
		{
			case 1:
				gpio_set(LED1, mmap_addr);
				gpio_clear(LED2, mmap_addr);
				gpio_clear(LED3, mmap_addr);
				printf("LED1 is on\n");
				break;
			case 2:
				gpio_clear(LED1, mmap_addr);
				gpio_set(LED2, mmap_addr);
				gpio_clear(LED3, mmap_addr);
				printf("LED2 is on\n");
				break;
			case 3:
				gpio_clear(LED1, mmap_addr);
				gpio_clear(LED2, mmap_addr);
				gpio_set(LED3, mmap_addr);
				printf("LED3 is on\n");
				break;
			case 4:
				gpio_set(LED1, mmap_addr);
				gpio_set(LED2, mmap_addr);
				gpio_set(LED3, mmap_addr);
				printf("all LEDs are on\n");
				break;
			case 0:
				gpio_clear(LED1, mmap_addr);
				gpio_clear(LED2, mmap_addr);
				gpio_clear(LED3, mmap_addr);
				printf("all LEDs are off\n");
				break;
		}
	}
}