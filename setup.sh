#!/bin/sh

# setup script

# download data if necessary
mkdir -p ~/data
if [[ ! -d ~/data/text8 ]]; then
  mkdir ~/data/text8
  pushd ~/data/text8 > /dev/null
  curl -OL http://timan102.cs.illinois.edu/~xwang95/data/text8/text8.gz
  gunzip text8.gz
  popd > /dev/null
fi

if [[ ! -d ~/data/gigaword ]]; then
  mkdir ~/data/gigaword
  pushd ~/data/gigaword > /dev/null
  curl -OL http://timan102.cs.illinois.edu/~xwang95/data/gigaword/giga_nyt.txt
  curl -OL http://timan102.cs.illinois.edu/~xwang95/data/gigaword/giga_nyt.vcb
  popd > /dev/null
fi

if [[ ! -d ~/data/w2v ]]; then
  mkdir ~/data/w2v
  pushd ~/data/w2v > /dev/null
  curl -OL \
    http://timan102.cs.illinois.edu/~xwang95/data/w2v/questions-phrases.txt
  curl -OL \
    http://timan102.cs.illinois.edu/~xwang95/data/w2v/questions-words.txt
  popd > /dev/null
fi
