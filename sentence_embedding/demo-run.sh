# make
# time ./s3e -train questions-2.filter.txt

# default options
file="aan_title.txt"
fvocab="aan_title.vocab"
threads=20
lr=1e-3
smn=100
syn=10
bestk=2
vocab=-1
iters=100
decay=1
dyndecay=1
testdecay=0.75
adagrad=0
shrink=1
save=10
test="word_similarity"
host=""
# server overwrite
if [[ $(hostname) == "timan101.cs.illinois.edu" ]]
then
  host="timan101"
  file="mr.txt"
  fvocab="mr.vocab"
  threads=20
  lr=1e-4
  part=1
elif [[  $(hostname) == "timan102.cs.illinois.edu" ]]
then
  host="timan102"
  threads=24
  part=10
  test="phrase_word"
  iters=1000
elif [[  $(hostname) == "timan103.cs.illinois.edu" ]]
then
  host="timan103"
  threads=24
  part=1
fi

# model file path generation
modeldecay=$decay
[[ $vocab     -eq -1  ]] && vocab_str="all"     || vocab_str="f$vocab"
[[ $adagrad   -eq 1   ]] && opt_str="adagrad"   || opt_str="gd"
[[ $shrink    -eq 1   ]] && sh_str="s"          || sh_str="ns"
[[ $dyndecay  -eq 1   ]] && dd_str="dd"         || dd_str="d"
[[ $1      ==  "test" ]] && decay=$testdecay
model_file="$file-$host-$smn-$syn-$bestk-$vocab_str\
s$save-t$iters-$opt_str-$sh_str-l$lr-$dd_str$modeldecay.model"
[[ $1      == "test"  ]] && model_file="$model_file.p$part"
[[ $1      == "test"  ]] && { make test; bin="./s3e_test"; aux="-test $test"; }
[[ $1      == "train" ]] && { make s3e; bin="./s3e"; aux=""; }
# option generation
opt="-train $file -vocab $fvocab -threads $threads \
     -smn $smn -syn $syn -bestk $bestk -vocab-size $vocab \
     -int-save $save -iters $iters -adagrad $adagrad -shrink-lr $shrink \
     -init-lr $lr -score-decay $decay -dynamic-score-decay $dyndecay"

time $bin $opt -model $model_file $aux
