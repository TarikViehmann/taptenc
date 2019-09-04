# adapted from https://stackoverflow.com/questions/48791883/best-practice-for-building-a-make-file/48793058#48793058
#          and http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/

BUILD := debug

COMPILER=gcc

DEPFLAGS = -MT $@ -MMD -MP -MF $(DEP_DIR)/$*.Td
POSTCOMPILE = mv -f $(DEP_DIR)/$*.Td $(DEP_DIR)/$*.d && touch $@

CXX.gcc := /bin/g++
CC.gcc := /bin/gcc
LD.gcc := /bin/g++
AR.gcc := /bin/ar

CXX.clang := /bin/clang++
CC.clang := /bin/clang
LD.clang := /bin/clang++
AR.clang := /bin/ar

CXX := ${CXX.${COMPILER}}
CC := ${CC.${COMPILER}}
LD := ${LD.${COMPILER}}
AR := ${AR.${COMPILER}}

CXXFLAGS.gcc.debug := -Og -fstack-protector-all
CXXFLAGS.gcc.release := -O3 -march=native -DNDEBUG
CXXFLAGS.gcc := -pthread -std=gnu++14 -march=native -W{all,extra,error} -g -fmessage-length=0 ${CXXFLAGS.gcc.${BUILD}}

CXXFLAGS.clang.debug := -O0 -fstack-protector-all
CXXFLAGS.clang.release := -O3 -march=native -DNDEBUG
CXXFLAGS.clang := -pthread -std=gnu++14 -march=native -W{all,extra,error} -g -fmessage-length=0 ${CXXFLAGS.clang.${BUILD}}

CXXFLAGS := ${CXXFLAGS.${COMPILER}}
CFLAGS := ${CFLAGS.${COMPILER}}

LDFLAGS.debug :=
LDFLAGS.release :=
LDFLAGS := -fuse-ld=gold -pthread -g ${LDFLAGS.${BUILD}}
LDLIBS := -ldl
ifeq ($(WITH_UTAP),1)
LDLIBS := $(LDLIBS) -L/usr/local/lib -lutap -lxml2
endif

COMPILE.CXX = ${CXX} -c -o $@ ${CPPFLAGS} ${CXXFLAGS} $(DEPFLAGS) $(abspath $<) $(SRC_DIRS:%.=-I$(BASE_DIR)/%)
PREPROCESS.CXX = ${CXX} -E -o $@ ${CPPFLAGS} ${CXXFLAGS} $(abspath $<)
COMPILE.C = ${CC} -c -o $@ ${CPPFLAGS} -MD -MP ${CFLAGS} $(abspath $<)
LINK.EXE = ${LD} -o $@ $(LDFLAGS) $(filter-out Makefile,$^) $(LDLIBS)
LINK.SO = ${LD} -shared -o $@ $(LDFLAGS) $(filter-out Makefile,$^) $(LDLIBS)
LINK.A = ${AR} rsc $@ $(filter-out Makefile,$^)
