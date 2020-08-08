# fts
Full text search experiments

We can currently tokenize a 5.6 GiB [Wikipedia abstract dump](https://dumps.wikimedia.org/enwiki/20200720/enwiki-20200720-abstract.xml.gz) in 5.6 s at 1.0 GiB/s on a 2019 MBP 2.4 GHz 8-Core Intel Core i9. Tokenizing the .xml input uses memory proportional to the longest span of text within or between tags. In this instance, we use ~8 KiB to tokenize the file.

Tokenizing and constructing the DOM tree in memory takes 40 s at ~140 MiB/s. The memory used is proportional to the size of the document itself: ~20 GiB here. This is due to the fact that each node in the tree has an overhead of 40 B, and also contains a copy of its data (tag name or text) from the original document. There's opportunity for improvement here.
