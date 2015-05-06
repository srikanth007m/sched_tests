################################################################################
# Copyright (C) 2015 ARM Ltd
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
################################################################################

docs_html:
	@echo "==== Update HTML docs ===="
	@cd docs && asciidoctor --out-file=index.html 00_main.adoc

clean_docs_html:
	@echo "==== Clean HTML docs ===="
	@rm -f docs/index.html

docs: docs_html
clean_docs: clean_docs_html

.PHONY: build_docs_html clean_docs_html


