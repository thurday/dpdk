#   BSD LICENSE
# 
#   Copyright(c) 2010-2012 Intel Corporation. All rights reserved.
#   Copyright(c) 2013 6WIND.
#   All rights reserved.
# 
#   Redistribution and use in source and binary forms, with or without 
#   modification, are permitted provided that the following conditions 
#   are met:
# 
#     * Redistributions of source code must retain the above copyright 
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright 
#       notice, this list of conditions and the following disclaimer in 
#       the documentation and/or other materials provided with the 
#       distribution.
#     * Neither the name of Intel Corporation nor the names of its 
#       contributors may be used to endorse or promote products derived 
#       from this software without specific prior written permission.
# 
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
#   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
#   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
#   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
#   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
#   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
#   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
#   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
#   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
#   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
#   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
#  version: DPDK.L.1.2.3-3

ifdef T
ifeq ("$(origin T)", "command line")
$(error "Cannot use T= with doc target")
endif
endif

.PHONY: help
help:
	@cat $(RTE_SDK)/doc/build-sdk-quick.txt
	@$(MAKE) -rR showconfigs | sed 's,^,\t\t\t\t,'

.PHONY: all
all: htmlapi

.PHONY: clean
clean: htmlapi-clean

.PHONY: htmlapi
htmlapi: htmlapi-clean
	@echo 'doxygen for API...'
	$(Q)mkdir -p $(RTE_OUTPUT)/doc/html
	$(Q)(cat $(RTE_SDK)/doc/doxy-api.conf         && \
	    echo OUTPUT_DIRECTORY = $(RTE_OUTPUT)/doc && \
	    echo HTML_OUTPUT      = html/api          && \
	    echo GENERATE_HTML    = YES               && \
	    echo GENERATE_LATEX   = NO                && \
	    echo GENERATE_MAN     = NO                )| \
	    doxygen -
	$(Q)$(RTE_SDK)/doc/doxy-html-custom.sh $(RTE_OUTPUT)/doc/html/api/doxygen.css

.PHONY: htmlapi-clean
htmlapi-clean:
	$(Q)rm -f $O/doc/html/api/*
	$(Q)rmdir -p --ignore-fail-on-non-empty $(RTE_OUTPUT)/doc/html/api 2>&- || true
