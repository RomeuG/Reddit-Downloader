all:
	gcc src/main.c src/cJSON/cJSON.c -o reddit-downloader -lcurl
