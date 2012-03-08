
$(LIBRARY): $(outdir)/config.hh $(LIB_O_FILES)
	$(AR) $(ARFLAGS) $@ $(LIB_O_FILES)
# thanks to Nelson Beebe for this trick.
	$(RANLIB) $@ || $(AR) ts $@ || true



