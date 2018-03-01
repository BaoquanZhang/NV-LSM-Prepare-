top_dir=.
include $(top_dir)/Makefile.config

SUBDIRS=src

.PHONY: all $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) --directory=$@

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) --directory=$$dir clean || exit 1; \
	done;
	$(RM) $(bin_dir)


