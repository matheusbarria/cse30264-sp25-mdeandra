SUBDIRS = nettest base64 index

all: $(SUBDIRS)

# Target to build each subdirectory
$(SUBDIRS):
	$(MAKE) -C $@

# Clean target
clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

.PHONY: all clean $(SUBDIRS)
