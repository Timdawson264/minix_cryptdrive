urlget http://192.168.1.15/~tim/getcryptdrive.php | gzip -d > cryptdrive.tar;

#cat cryptdrive.tar | tar -tf - | grep '\./.' | while read x; do rm -rf $x; done;

cat cryptdrive.tar | tar -xf - ;

cat cryptdrive.tar | tar -tf - | grep '\./.' | while read x; do chown bin:0 $x; done;

rm cryptdrive.tar.gz;
