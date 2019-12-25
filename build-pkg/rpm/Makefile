

#need : VERSION(git commit) RELEASE(date)
PWD=$(shell pwd)
RPMBUILD=./rpmbuild
SPEC_FILE=./tapplet.spec

all: RPM

spec:
	@echo $(TARBALL)
	rm -rf $(RPMBUILD)
	mkdir -p $(RPMBUILD)/{RPMS,SRPMS,BUILD,SOURCES,SPECS}
	cp $(SPEC_FILE) $(RPMBUILD)/SPECS

RPM: spec
	@cp -r ../build-native/* $(RPMBUILD)/BUILD/
	rpmbuild -bb \
	  --define "_topdir $(PWD)/$(RPMBUILD)" \
	  --define "_release $(_RELEASE)" \
	  --define "_version $(_VERSION)" \
	  $(RPMBUILD)/SPECS/$(SPEC_FILE)
	mv $$(find $(RPMBUILD)/RPMS -name \*.rpm -type f) ./
