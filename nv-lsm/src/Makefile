top_dir=..
cur_dir=.
include $(top_dir)/Makefile.config

.PHONY: all dirs

SRCS  = $(wildcard *.cpp)
OBJS  = $(patsubst %,$(obj_dir)/%,$(patsubst %.cpp,%.o,$(notdir $(SRCS))))
DEPS := $(OBJS:.o=.d)

-include $(DEPS)

all: $(PROG)

$(PROG): dirs $(OBJS)
	$(CXX) -o $(bin_dir)/$(PROG) $(OBJS) $(LDFLAGS)

$(obj_dir)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -w $< -o $@

clean:
	$(RM) $(obj_dir)

