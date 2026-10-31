FILM_ENGINE=../../soup-engine/src/engine
IXFILM_LIBGCC=$(shell $(CC) -print-libgcc-file-name)

FLTS += $Oixfilm.flt

$Oixfilm.flt: $Oixfilm.o $Oixfilm_jpeg_dec.o $Oixfilm_jpeg_enc.o \
	$Oixfilm_stock.o $Oixfilm_look.o $Oixfilm_grade.o \
	$Oixfilm_wb.o $(IXFILM_LIBGCC)

$Oixfilm.o: ixfilm.c
	@echo $< '->' $@
	$(CC) $(CFLAGS) -Iixfilm_compat -I$(FILM_ENGINE) -c -o $@ $<

$Oixfilm_jpeg_dec.o: $(FILM_ENGINE)/jpeg_dec.c
	@echo $< '->' $@
	$(CC) $(CFLAGS) -Iixfilm_compat -I$(FILM_ENGINE) -c -o $@ $<

$Oixfilm_jpeg_enc.o: $(FILM_ENGINE)/jpeg_enc.c
	@echo $< '->' $@
	$(CC) $(CFLAGS) -Iixfilm_compat -I$(FILM_ENGINE) -c -o $@ $<

$Oixfilm_stock.o: $(FILM_ENGINE)/stock.c
	@echo $< '->' $@
	$(CC) $(CFLAGS) -Iixfilm_compat -I$(FILM_ENGINE) -c -o $@ $<

$Oixfilm_look.o: $(FILM_ENGINE)/look.c
	@echo $< '->' $@
	$(CC) $(CFLAGS) -Iixfilm_compat -I$(FILM_ENGINE) -c -o $@ $<

$Oixfilm_grade.o: $(FILM_ENGINE)/grade.c
	@echo $< '->' $@
	$(CC) $(CFLAGS) -Iixfilm_compat -I$(FILM_ENGINE) -c -o $@ $<

$Oixfilm_wb.o: $(FILM_ENGINE)/wb.c
	@echo $< '->' $@
	$(CC) $(CFLAGS) -Iixfilm_compat -I$(FILM_ENGINE) -c -o $@ $<

