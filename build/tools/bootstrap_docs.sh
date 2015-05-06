#!/bin/bash
################################################################################
# Copyright (C) 2015 ARM Ltd
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2, as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
################################################################################

DOCTEMPATE="$(pwd)/build/tools/docs_template.tar.xz"
DOCROOT="$(pwd)/docs"

echo "Cleanup previous generated links"
rm $DOCROOT/04_TestSuites_*.adoc
rm $DOCROOT/04_TestSuites_suites.adoc
find docs/images -type l | xargs rm
find docs/suites -type l | xargs rm

echo "Build doc template"
rm -f docs_template.tar.xz
pushd build/tools >/dev/null
tar cJf $DOCTEMPATE docs_template
popd >/dev/null

add_tests() {
  TESTS=$(ls testcases)

  for T in $TESTS; do
    echo "Add testcase [$T]..."
    cat > docs/$T.adoc <<EOF

[[test_${T}]]
==== $T

EOF

    cat testcases/$T/desc.meta | \
    awk '/run: /{next}/result: /{next}{print $0}' | \
    sed 's/^description: //' >> docs/$T.adoc

    cat >> docs/$T.adoc <<EOF

.Detailed Description
The test starts by...
EOF


    echo -e "\ninclude::./$T.adoc[]" >> docs/00_main.adoc

  done

  echo -e "\n\n// vim: set syntax=asciidoc:" >> docs/00_main.adoc
}

add_suite() {
  SUITE_DIR=${DIR/\/testcases/}
  SUITE=$(echo $SUITE_DIR | sed -r "s|.*/(.*)_suite|\1|g")
  SUITE_NAME=${SUITE/_/ }

  echo
  echo
  echo "Add suite $SUITE_DIR..."
  pushd $SUITE_DIR >/dev/null

  rm -rf docs
  tar xJf $DOCTEMPATE
  mv docs_template docs

  pushd docs >/dev/null
  SEDSCRPT="s|suitename|${SUITE}|;s|SuiteName|${SUITE_NAME^}|;s|SUITENAME|${SUITE^^}|"
  echo  "  updating tokens [$SEDSCRPT]"
  sed -i "$SEDSCRPT" 00_main.adoc
  echo "  updating image dir to [images/$SUITE]..."
  mv images/suitename images/$SUITE
  echo "  generate local documentation"
  asciidoctor 00_main.adoc
  popd >/dev/null

  add_tests

  echo "  link to global documentation"

  pushd $DOCROOT/suites >/dev/null
  ln -s ../../$SUITE_DIR/docs $SUITE
  popd >/dev/null

  pushd $DOCROOT/images >/dev/null
  ln -s ../../$SUITE_DIR/docs/images/$SUITE
  popd >/dev/null


  SUITE_CATEGORY=""
  [[ $(dirname $SUITE_DIR) =~ suites/platform ]] && SUITE_CATEGORY="platform"
  [[ $(dirname $SUITE_DIR) =~ suites/sched/hmp.*checks.* ]] && SUITE_CATEGORY="hmp_check"
  [[ $(dirname $SUITE_DIR) =~ suites/sched/hmp.*basic.* ]] && SUITE_CATEGORY="hmp_basic"
  [[ $(dirname $SUITE_DIR) =~ suites/sched/hmp.*advanced.* ]] && SUITE_CATEGORY="hmp_advanced"
  [[ $(dirname $SUITE_DIR) =~ suites/stability ]] && SUITE_CATEGORY="stability"

  echo "  add suite [$SUITE_DIR] documentation to [$SUITE_CATEGORY] cetegory"

  SUITE_FRAGMENT="$DOCROOT/04_TestSuites_$SUITE_CATEGORY.adoc"
  # echo "include::./suites/$SUITE/00_main.adoc[]" >> $SUITE_FRAGMENT
  echo "* <<suite_$SUITE,$SUITE>>" >> $SUITE_FRAGMENT

  echo "include::./suites/$SUITE/00_main.adoc[]" >> $DOCROOT/04_TestSuites_all.adoc

  popd >/dev/null

}

CATEGORIES=" \
suites/platform \
suites/sched/hmp_01_checks \
suites/sched/hmp_02_basic_features \
suites/sched/hmp_03_advanced_features \
suites/stability \
"

for CAT in $CATEGORIES; do \
  find $CAT -name testcases | \
  while read DIR; do add_suite $DIR; done
done

### Compile documenation
pushd docs >/dev/null
asciidoctor --out-file=index.html 00_main.adoc
xdg-open index.html &
popd >/dev/null

### Generate documentation archive
tar cJhf SchedTestDocs.tar.xz --exclude="suites/*/images" --exclude="suites/*/*.html" SchedTestSuiteDocs

