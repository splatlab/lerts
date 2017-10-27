# popcorning

To run the program.

```bash
$ make popcornfilter
$ ./popcornfilter 16 4 4 2 4
```

Here is an explanation of the arguments:
* In-memory CQF size: The log of the number of slots in the in-memory CQF
* NumLevels: Number of levels in each cone or cascade filter
* Growth factor: The ratio of the number of slots in level i+1 and i in each casacade filter
* NumCones: Number of cones or cascade filters
* NumThreads: Number of threads to spawn for insertion
