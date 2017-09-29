
LANGS   = cs de es fr hu it ja pl pt_BR
POCOMP  = ./pocomp.pl
HARVEST = ./harvest.pl

DICTS  = eboard.cs.dict eboard.de.dict eboard.es.dict eboard.fr.dict eboard.hu.dict eboard.it.dict eboard.ja.dict eboard.pl.dict eboard.pt_BR.dict

dicts: $(DICTS)

%.dict : %.po
	$(POCOMP) < $< > $@

update:
	$(HARVEST)

# all targets below this point should be called from the parent directory's
# Makefile, or the vars they use will have bogus values

install-nls: $(DICTS)
	$(INSTALL) -d $(datadir)
	$(INSTALL) -m 0644 $(DICTS) $(datadir)

uninstall-nls:
	for p in $(DICTS); do rm -f $(datadir)/$$p ; done

EXTRADIST = README Makefile pocomp.pl harvest.pl harvest.in eboard.hemp eboard.cs.po eboard.de.po eboard.es.po eboard.fr.po eboard.hu.po eboard.it.po eboard.ja.po eboard.pl.po eboard.pt_BR.po

dist: $(DICTS)
	cp -f $(DICTS) $(EXTRADIST) ../$(DIST)/multilang

