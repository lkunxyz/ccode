t=`date +%Y/%m/%d-%X`
comment="commit time:$t"

#echo $comment

echo '-----------   git add .   -------------------'
git add .

echo
echo
echo "--- git commit -m $comment -------"
#echo "$comment"
git commit -m "$comment"


echo
echo
echo '---  git push -u origin source --------------'
git push -u origin source


echo
echo
echo '------ All operations excute end.------------'cho $time
