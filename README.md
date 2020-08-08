# fts
Full text search experiments

We can currently tokenize the [Wikipedia abstract dump](https://dumps.wikimedia.org/enwiki/20200720/enwiki-20200720-abstract.xml.gz) at 1.2 GiB/s.
```
~/fts master* ⇡
❯ ls -l enwiki-20200720-abstract.xml
-rw-r--r--@ 1 amastro  staff  6072016532 Aug  6 21:42 enwiki-20200720-abstract.xml

~/fts master* ⇡
❯ make && time ./fts enwiki-20200720-abstract.xml
cc -std=c11 -Wall -Wextra -Wpedantic -g -O3 main.c -o fts
./fts enwiki-20200720-abstract.xml  4.54s user 0.96s system 97% cpu 5.625 total
```
Tokenizing the .xml input uses memory proportional to the largest span of text within or between tags. In this instance, we use ~8 KiB at most to tokenize the 5.6 GiB file.
