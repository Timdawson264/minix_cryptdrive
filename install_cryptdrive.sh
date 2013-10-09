urlget http://192.168.1.15/~tim/getcryptdrive.php>cryptdrive.tar.gz;

cat cryptdrive.tar.gz | gzip -d | tar -tf - | grep '\./.' | while read x; do echo $x; done;

cat cryptdrive.tar.gz | gzip -d | tar -xf -;
