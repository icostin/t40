N := t40
D := T40

engine_csrcs := world misc
engine_libs := -lc41
engine_pub_hdrs := include/t40.h
engine_priv_hdrs := src/engine.h
engine_dl_opts := -ffreestanding -nostartfiles -nostdlib -Wl,-soname,lib$(N).so

clitool_csrcs := clitool test
clitool_libs := -lc41 -lhbs1clid -lhbs1

########
cflags := -fvisibility=hidden -Iinclude -Wall -Wextra -Werror $(cflags_ext)
cflags_sl := $(cflags) -D$(D)_STATIC
cflags_dl := $(cflags) -D$(D)_DLIB_BUILD -fpic
cflags_rls := -DNDEBUG
cflags_dbg := -D_DEBUG

engine_dl_rls_objs := $(patsubst %,out/engine-dl-rls/%.o,$(engine_csrcs))
engine_dl_dbg_objs := $(patsubst %,out/engine-dl-dbg/%.o,$(engine_csrcs))

clitool_dl_objs := $(patsubst %,out/clitool-dl/%.o,$(clitool_csrcs))

ifeq ($(PREFIX_DIR),)
PREFIX_DIR:=$(HOME)/.local
endif

.PHONY: all clean tags arc engines engine-dl-rls engine-dl-dbg clitools clitool-dl install uninstall test

all: engines clitools

arc:
	cd .. && tar -Jcvf $(N).txz $(N)/src $(N)/include $(N)/make* $(N)/README* $(N)/LICENCE

install: engine-dl-rls clitool-dl
	mkdir -p $(PREFIX_DIR)/bin
	mkdir -p $(PREFIX_DIR)/lib
	mkdir -p $(PREFIX_DIR)/include
	cp -v out/engine-dl-rls/lib$(N).so $(PREFIX_DIR)/lib/
	[ `whoami` != root ] || ldconfig
	cp -v out/clitool-dl/t40 $(PREFIX_DIR)/bin/t40
	cp -vr include/t40.h $(PREFIX_DIR)/include/t40.h

uninstall:
	-rm -f $(PREFIX_DIR)/lib/lib$(N).so
	-rm -rf $(PREFIX_DIR)/include/t40.h
	[ `whoami` != root ] || ldconfig

clean:
	-rm -rf out tags

tags:
	ctags -R --fields=+iaS --extra=+q --exclude='.git' .

engines: engine-dl-rls engine-dl-dbg

clitools: clitool-dl

test: engines clitool-dl
	LD_LIBRARY_PATH=out/engine-dl-dbg:$(LD_LIBRARY_PATH) out/clitool-dl/$(N) -t
	LD_LIBRARY_PATH=out/engine-dl-rls:$(LD_LIBRARY_PATH) out/clitool-dl/$(N) -t

engine-dl-rls: out/engine-dl-rls/lib$(N).so

engine-dl-dbg: out/engine-dl-dbg/lib$(N).so

clitool-dl: out/clitool-dl/$(N)

# dirs
out out/engine-dl-rls out/engine-dl-dbg out/clitool-dl:
	mkdir -p $@

# dynamic libs
out/engine-dl-rls/libt40.so: $(engine_dl_rls_objs) | out/engine-dl-rls
	gcc -shared	-o$@ $(engine_dl_opts) $(cflags_dl) $^ $(engine_libs)

out/engine-dl-dbg/libt40.so: $(engine_dl_dbg_objs) | out/engine-dl-dbg
	gcc -shared	-o$@ $(engine_dl_opts) $(cflags_dl) $^ $(engine_libs)

# object files for dynamic release lib
$(engine_dl_rls_objs): out/engine-dl-rls/%.o: src/%.c $(engine_pub_hdrs) $(engine_priv_hdrs) | out/engine-dl-rls
	gcc -c -o$@ $< -Iinclude $(cflags_dl) $(cflags_rls)

# object files for dynamic debug lib
$(engine_dl_dbg_objs): out/engine-dl-dbg/%.o: src/%.c $(engine_pub_hdrs) $(engine_priv_hdrs) | out/engine-dl-dbg
	gcc -c -o$@ $< -Iinclude $(cflags_dl) $(cflags_dbg)

# command line tool (dynamic linked)
out/clitool-dl/$(N): $(patsubst %,src/%.c,$(clitool_csrcs)) $(engine_pub_hdrs) out/engine-dl-rls/lib$(N).so | out/clitool-dl
	gcc -o$@ $(patsubst %,src/%.c,$(clitool_csrcs)) $(cflags) -Lout/engine-dl-rls -l$(N) $(clitool_libs)

