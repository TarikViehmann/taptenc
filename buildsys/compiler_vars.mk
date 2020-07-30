# adapted from https://stackoverflow.com/questions/48791883/best-practice-for-building-a-make-file/48793058#48793058
#          and http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/

BUILD := debug

DEPFLAGS = -MT $@ -MMD -MP -MF $(DEP_DIR)/$*.Td
POSTCOMPILE = mv -f $(DEP_DIR)/$*.Td $(DEP_DIR)/$*.d && touch $@

CXXFLAGS.debug := -Og -fstack-protector-all -g
CXXFLAGS.release := -O3 -march=native -DNDEBUG
CXXFLAGS := -pthread -std=gnu++14 -W{all,extra,error} -fmessage-length=0 ${CXXFLAGS.${BUILD}}

LDFLAGS.debug :=
LDFLAGS.release :=
LDFLAGS := -pthread ${LDFLAGS.${BUILD}}
LDLIBS := -ldl -lc
ifeq ($(WITH_UTAP),1)
LDLIBS := $(LDLIBS) -L/usr/local/lib -lutap -lxml2
endif

COMPILE.CXX = ${CXX} -c -o $@ ${CPPFLAGS} ${CXXFLAGS} $(DEPFLAGS) $(abspath $<) $(SRC_DIRS:%.=-I$(BASE_DIR)/%)
PREPROCESS.CXX = ${CXX} -E -o $@ ${CPPFLAGS} ${CXXFLAGS} $(abspath $<)
COMPILE.C = ${CC} -c -o $@ ${CPPFLAGS} -MD -MP ${CFLAGS} $(abspath $<)
LINK.EXE = ${CXX} -o $@ $(LDFLAGS) $(filter-out Makefile,$^) $(LDLIBS)
LINK.SO = ${CXX} -shared -o $@ $(LDFLAGS) $(filter-out Makefile,$^) $(LDLIBS)
LINK.A = ${AR} rsc $@ $(filter-out Makefile,$^)
