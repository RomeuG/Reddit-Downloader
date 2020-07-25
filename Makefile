all:
	gcc src/main.c src/cJSON/cJSON.c -o reddit-downloader -lcurl

fast:
	gcc -Ofast -mavx2 -march=native src/main.c src/cJSON/cJSON.c -o reddit-downloader -lcurl
