t=`date +%Y/%m/%d-%X`
comment="commit at:$t"

#echo $comment

echo '-----------   git add .   -------------------'
git add .

echo
echo
echo "--- git commit -m $comment -------"
git commit -m "$comment"


echo
echo
echo '---  git push -u origin master --------------'
git push -u origin master


echo
echo
echo '------ All operations excute end.------------' 

#echo $time
