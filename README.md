Reddit-Downloader
=================

Tool created to download Reddit thread's JSON for archival purposes.

Features (WIP)
==============

- Download thread's JSON from a specific subreddit (limit of ~1000)

Configuration
=============

Configuration is made entirely through the following self-explanatory Environment Variables:

- `REDDIT_ID`
- `REDDIT_SECRET`
- `REDDIT_USER`
- `REDDIT_PASS`
- `REDDIT_USERAGENT`

Example
=======

Imagine you want to archive the `Emacs` subreddit. You do the following:

```
$ ./reddit-downloader -r emacs -o OUTPUT_DIR
```

If the command is ran multiple times to the same directory, duplicates will be generated.
To delete the duplicates and to actually keep the most up to date JSON of the threads, you just run the script in the `scripts` directory:

```
$ python3 scripts/duplicate_remover.py OUTPUT_DIR
```
