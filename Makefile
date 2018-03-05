TARGET = nvram-liber-macos
KERN_UTILS = ios-kern-utils
XCFLAGS ?= -I $(KERN_UTILS)/src/lib
XLFLAGS ?= -framework CoreFoundation

.PHONY: all clean

all: bin/$(TARGET)

bin/$(TARGET): src/*.c $(KERN_UTILS)/libkutil.a | bin
	$(CC) -o $@ $^ $(XCFLAGS) $(XLFLAGS)

bin:
	mkdir -p $@

$(KERN_UTILS)/libkutil.a: $(KERN_UTILS)
	make -C $< lib

clean:
	rm -rf bin
	make -C $(KERN_UTILS) clean

