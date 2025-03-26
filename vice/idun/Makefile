RESC=resc
X128=x128
X64=x64sc
IDUNHOST=127.0.0.1:25232

EMUROM=$(RESC)/emu.rom \
$(RESC)/emu64.rom \

all: $(RESC) $(EMUROM)

$(EMUROM): strap.asm
	acme -o $(RESC)/emu.rom -f plain -Dcomputer=128 -DROMSIZE=16384 strap.asm
	acme -o $(RESC)/emu64.rom -f plain -Dcomputer=64 -DROMSIZE=8192 strap.asm

$(RESC):
	mkdir -p $@

emu: resc
	$(X128) -pal -80col -reu -idunhost $(IDUNHOST) -idunio -extfunc 1 -extfrom $(RESC)/emu.rom

go64: resc
	$(X64) -pal -idunhost $(IDUNHOST) -idunio -cart8 $(RESC)/emu64.rom

clean:
	rm -fr $(RESC)
