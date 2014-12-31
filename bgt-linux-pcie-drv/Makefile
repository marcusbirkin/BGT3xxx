BUILD_DIR := $(shell pwd)/v4l
TMP ?= /tmp
REPO_PULL := http://linuxtv.org/hg/v4l-dvb

ifeq ($(EDITOR),)
  ifeq ($(VISUAL),)
	EDITOR := vi
  else
	EDITOR := $(VISUAL) -w
  endif
endif

all:

install:
	$(MAKE) -C $(BUILD_DIR) install

# Hmm, .PHONY does not work with wildcard rules :-(
SPECS = v4l2-spec dvb-spec

.PHONY: $(SPECS)

$(SPECS):
	$(MAKE) -C $(BUILD_DIR) $(MAKECMDGOALS)

%::
	$(MAKE) -C $(BUILD_DIR) $(MAKECMDGOALS)

commit cvscommit hgcommit change changes changelog:: whitespace
	@cd $(BUILD_DIR); scripts/cardlist; scripts/do_commit.sh $(EDITOR) $(TMP)/v4l_hg_whitespace; cd ..

qrefresh: Q=q
qrefresh:: whitespace
	cd $(BUILD_DIR); scripts/cardlist; cd ..
	v4l/scripts/prep_commit_msg.pl -q $(TMP)/v4l_hg_whitespace > \
		$(TMP)/v4l_hg_commit.msg
	$(EDITOR) $(TMP)/v4l_hg_commit.msg
	grep -v '^#' $(TMP)/v4l_hg_commit.msg | hg qrefresh -g -l -

pull update v4l-update::
	@echo "Pulling changes from master repository $(REPO_PULL)"
	-hg pull -u $(REPO_PULL)

push::
	@echo "Pushing changes to master repository"
	-hg push

whitespace whitespaces:
	@echo "Cleaning bad whitespaces"
	@v4l/scripts/strip-trailing-whitespaces.sh $(Q)fast | \
		tee $(TMP)/v4l_hg_whitespace | patch -p0
