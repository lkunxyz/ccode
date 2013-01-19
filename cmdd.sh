time=`date +%Y/%m/%d-%X`



echo '-----------   git add .   -------------------'
git add .

echo
echo
echo '--- git commit -m message for commit $time-------'
git commit -m 'commit message'


echo
echo
echo '---  git push -u origin master --------------'
git push -u origin master


echo
echo
echo '------ All operations excute end.------------'echo $time

