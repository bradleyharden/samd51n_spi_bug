CC = "arm-none-eabi-gcc"
CP = "arm-none-eabi-objcopy"

LINKER = base/samd51n20a_flash.ld

INCLUDES = \
-I./\
-I./include \
-I./include/core\

MACROS  = -D __SAMD51N20A__
CFLAGS  = $(INCLUDES) -std=c99 -fno-common -Os -g -mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 --specs=nano.specs -Wl,--gc-sections
LFLAGS  = -T $(LINKER) 
CPFLAGS = -O binary

NAME = main

SOURCES = \
	$(NAME).c\
	base/startup_samd51.c\
	base/system_samd51.c\

all: reg

clean: 
	rm -f $(NAME).elf $(NAME).bin $(NAME).dmp *.list *.o

reg: $(NAME).elf
	$(CP) $(CPFLAGS) $(NAME).elf $(NAME).bin

$(NAME).elf: $(SOURCES) $(LINKER) 
	$(CC) $(MACROS) $(CFLAGS) $(LFLAGS) -o $(NAME).elf  $(SOURCES)

