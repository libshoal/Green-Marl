SHELL=bash
MKDIR=mkdir
CONFIG_FILE=setup.mk
CPP=apps/output_cpp
LIB=apps/output_cpp/gm_graph
GPS=apps/output_gps
GIRAPH=apps/output_giraph
BUILD_DIRS=\
	bin \
	obj \
	$(CPP)/bin \
	$(LIB)/javabin \
	$(CPP)/generated \
	$(CPP)/data \
	$(LIB)/lib \
	$(LIB)/obj \
	$(GPS)/generated \
	$(GIRAPH)/generated \
	$(GIRAPH)/bin \
	$(GIRAPH)/java_bin \
	$(GIRAPH)/target

TEST_DIRS=\
	test/cpp_be/output \
	test/errors/output \
	test/gps/output \
	test/giraph/output \
	test/opt/output \
	test/parse/output \
	test/rw_check/output \
	test/sugars/output

PWD := $(shell pwd)

.PHONY: dirs compiler apps coverage
all: dirs $(CONFIG_FILE) compiler apps

$(CONFIG_FILE): setup.mk.in etc/update_setup
	@echo "Re-creating setup.mk from updated setup.mk.in. Please check setup.mk afterwrd";
	@if [ -f setup.mk ];   \
	then \
		cp setup.mk setup.mk.bak; \
	else \
		touch setup.mk.bak; \
	fi; \
	etc/update_setup setup.mk.bak setup.mk.in $(CONFIG_FILE) ${PWD}

compiler: dirs $(CONFIG_FILE) $(wildcard src/inc/*.h)
	@cd src; $(MAKE)

apps: dirs compiler $(CONFIG_FILE)
	@cd apps; $(MAKE)

coverage:
	rm -rf coverage coverage.info
	lcov --no-external --capture -b src --directory . --output-file coverage.info
	genhtml coverage.info --output-directory coverage

dirs: $(BUILD_DIRS) $(TEST_DIRS) $(CONFIG_FILE)

clean: $(CONFIG_FILE)
	@cd apps; $(MAKE) clean_all
	@cd src; $(MAKE) clean

clean_all: veryclean

veryclean: $(CONFIG_FILE)
	@cd apps; $(MAKE) clean_all
	@cd src; $(MAKE) veryclean
	rm -rf $(BUILD_DIRS) $(TEST_DIRS)

clean_coverage:
	rm -rf coverage coverage.info
	rm -rf obj/*.gcda obj/*.gcno

$(BUILD_DIRS):
	$(MKDIR) $@

$(TEST_DIRS):
	$(MKDIR) $@

etc/update_setup :
	g++ etc/update_setup.cc -o etc/update_setup

# --------------------------------------------------
#

GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always)

sk_pagerank: sk_pr_gm | sk_pr_gcc

ROOT := $(shell pwd)
BASE := $(ROOT)/shoal/
SHOAL := $(BASE)/shoal/

include $(SHOAL)/common.mk

INC := \
	-I$(SHOAL)/inc \
	-I$(BASE)/contrib/pycrc/

LIB := \
	-L$(SHOAL) -lshl

OBJS :=

# Switch buildtype: supported are "debug" and "release"
BUILDTYPE := release
FLAGS=$(CXXFLAGS) -Wno-unused-variable

ifeq ($(BUILDTYPE),debug)
	FLAGS += -O0  -pg -ggdb -DSHL_DEBUG
else
	FLAGS +=  -O3 -g
endif

FLAGS += -DVERSION=\"$(GIT_VERSION)\" -Wall
GMDIR = $(ROOT)

sk_clean:
	$(MAKE) -C $(SHOAL) clean
	$(MAKE) -C $(SHOAL)
	$(MAKE) -C $(ROOT)/src clean
	$(MAKE) compiler -j $(shell nproc)

.PHONY: sk_shoal
sk_shoal:
	$(MAKE) -C $(SHOAL) BUILDTYPE=$(BUILDTYPE)

sk_pr_gm:
	rm -rf apps/output_cpp/generated/pagerank.cc
	$(MAKE) -C apps/src/ ../output_cpp/generated/pagerank.cc

sk_pr_gcc: sk_shoal
	$(MAKE) -C $(SHOAL)
	cd apps/output_cpp/src; g++ $(FLAGS) $(INC) -I../generated -I../gm_graph/inc -I. -fopenmp -DDEFAULT_GM_TOP="\"$GMDIR\"" -std=gnu++0x -DAVRO ../generated/pagerank.cc pagerank_main.cc $(OBJS) ../gm_graph/lib/libgmgraph.a $(LIB) -o ../bin/pagerank

sk_pr_team_gcc: sk_shoal
	$(MAKE) -C $(SHOAL)
	cd apps/output_cpp/src; g++ $(FLAGS) $(INC) -I../generated -I../gm_graph/inc -I. -fopenmp -DDEFAULT_GM_TOP="\"$GMDIR\"" -std=gnu++0x -DAVRO pagerank.team.cc pagerank_main.cc $(OBJS) ../gm_graph/lib/libgmgraph.a $(LIB) -o ../bin/pagerank.team

sk_triangle_counting: sk_tc_gm | sk_tc_gcc

sk_tc_gm:
	rm -rf apps/output_cpp/generated/triangle_counting.cc
	$(MAKE) -C apps/src/ ../output_cpp/generated/triangle_counting.cc

sk_tc_gcc:
	$(MAKE) -C $(SHOAL)
	cd apps/output_cpp/src; g++ $(FLAGS) $(INC) -I../generated -I../gm_graph/inc -I. -fopenmp -DDEFAULT_GM_TOP="\"$GMDIR\"" -std=gnu++0x -DAVRO ../generated/triangle_counting.cc triangle_counting_main.cc $(OBJS) ../gm_graph/lib/libgmgraph.a -L../gm_graph/lib -lgmgraph $(LIB) -o ../bin/triangle_counting

sk_hop_dist: sk_hd_gm | sk_hd_gcc

sk_hd_gm:
	rm -rf apps/output_cpp/generated/hop_dist.cc
	$(MAKE) -C apps/src/ ../output_cpp/generated/hop_dist.cc

sk_hd_gcc:
	$(MAKE) -C $(SHOAL)
	cd apps/output_cpp/src; g++ $(FLAGS) $(INC) -I../generated -I../gm_graph/inc -I. -fopenmp -DDEFAULT_GM_TOP="\"$GMDIR\"" -std=gnu++0x -DAVRO ../generated/hop_dist.cc hop_dist_main.cc $(OBJS) ../gm_graph/lib/libgmgraph.a -L../gm_graph/lib -lgmgraph $(LIB) -o ../bin/hop_dist

sk_hd_ec_gcc:
	$(MAKE) -C $(SHOAL)
	cd apps/output_cpp/src; g++ $(FLAGS) $(INC) -I../generated -I../gm_graph/inc -I. -fopenmp -DDEFAULT_GM_TOP="\"$GMDIR\"" -std=gnu++0x -DAVRO ../generated/hop_dist_ec.cc hop_dist_main.cc $(OBJS) ../gm_graph/lib/libgmgraph.a -L../gm_graph/lib -lgmgraph $(LIB) -o ../bin/hop_dist

# --------------------------------------------------
# Compile from scratch. We need this for buildbot
# --------------------------------------------------

# Green Marl compiler
# --------------------
COMP_EXEC := bin/gm_comp

$(COMP_EXEC):
	make compiler

# Green Marl runtime
# --------------------
GMRT_BASE := apps/output_cpp/gm_graph
GMRT := $(GMRT_BASE)/lib/libgmgraph.a

$(GMRT):
	make -C $(GMRT_BASE)

# Build everything
# --------------------------------------------------
sk_buildbot: $(COMP_EXEC) $(GMRT)
