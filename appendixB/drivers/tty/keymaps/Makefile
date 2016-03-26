# Generate binary keymaps.

LK =	/usr/lib/keymaps

.SUFFIXES:	.src .map

.src.map:
	$(CC) -DKEYSRC=\"$<\" genmap.c
	./a.out > $@
	@rm -f a.out

all:	\
	us-std.map \

install:	\
	$(LK) \
	$(LK)/us-std.map \

clean:
	rm -f a.out *.map

$(LK):
	install -d $@

$(LK)/us-std.map:	us-std.map
	install -c $? $@
