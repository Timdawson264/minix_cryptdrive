urlget http://192.168.1.15/~tim/getcryptdrive.php | gzip -cryptdrive.tar;

#cat cryptdrive.tar | tar -tf - | grep '\./.' | while read x; do rm -rf $x; done;

cat cryptdrive.tar | tar -xf - ;

rm cryptdrive.tar.gz;
